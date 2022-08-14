/*
Copyright(c) 2016-2022 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

//= INCLUDES ===============================
#include "pch.h"
#include "World.h"
#include "Entity.h"
#include "Components/Transform.h"
#include "Components/Camera.h"
#include "Components/Light.h"
#include "Components/Environment.h"
#include "Components/AudioListener.h"
#include "Components/Renderable.h"
#include "TransformHandle/TransformHandle.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/ProgressTracker.h"
#include "../IO/FileStream.h"
#include "../Profiling/Profiler.h"
#include "../Input/Input.h"
#include "../RHI/RHI_Device.h"
#include "../Rendering/Renderer.h"
#include "../Threading/Threading.h"
#include "Components/AudioSource.h"
#include "Components/RigidBody.h"
#include "Components/Collider.h"
//==========================================

//= NAMESPACES ================
using namespace std;
using namespace Spartan::Math;
//=============================

namespace Spartan
{
    World::World(Context* context) : Subsystem(context)
    {
        SP_SUBSCRIBE_TO_EVENT(EventType::WorldResolve, [this](Variant) { m_resolve = true; });
    }

    World::~World()
    {
        m_input    = nullptr;
        m_profiler = nullptr;
    }

    void World::OnInitialise()
    {
        m_input    = m_context->GetSubsystem<Input>();
        m_profiler = m_context->GetSubsystem<Profiler>();
    }

    void World::OnPostInitialise()
    {
        m_transform_handle = make_shared<TransformHandle>(m_context);

        CreateDefaultWorld();
    }

    void World::OnPreTick()
    {
        for (shared_ptr<Entity>& entity : m_entities)
        {
            entity->OnPreTick();
        }
    }

    void World::OnTick(double delta_time)
    {
        // If something is being loaded, don't tick as entities are probably being added
        if (IsLoading())
            return;

        SCOPED_TIME_BLOCK(m_profiler);

        if (Renderer* renderer = m_context->GetSubsystem<Renderer>())
        {
            if (renderer->GetOption<bool>(RendererOption::Debug_TransformHandle))
            {
                m_transform_handle->Tick(renderer->GetCamera().get(), m_gizmo_transform_size);
            }
        }

        // Tick entities
        {
            // Detect game toggling
            const bool started   = m_context->m_engine->EngineMode_IsSet(Engine_Game)  && m_was_in_editor_mode;
            const bool stopped   = !m_context->m_engine->EngineMode_IsSet(Engine_Game) && !m_was_in_editor_mode;
            m_was_in_editor_mode = !m_context->m_engine->EngineMode_IsSet(Engine_Game);

            // Start
            if (started)
            {
                for (shared_ptr<Entity>& entity : m_entities)
                {
                    entity->OnStart();
                }
            }

            // Stop
            if (stopped)
            {
                for (shared_ptr<Entity>& entity : m_entities)
                {
                    entity->OnStop();
                }
            }

            // Tick
            for (shared_ptr<Entity>& entity : m_entities)
            {
                entity->Tick(delta_time);
            }
        }

        if (m_resolve)
        {
            // Update dirty entities
            {
                // Make a copy so we can iterate while removing entities
                vector<shared_ptr<Entity>> entities_copy = m_entities;

                for (shared_ptr<Entity>& entity : entities_copy)
                {
                    if (entity->IsPendingDestruction())
                    {
                        _EntityRemove(entity);
                    }
                }
            }

            // Notify Renderer
            SP_FIRE_EVENT_DATA(EventType::WorldResolved, m_entities);
            m_resolve = false;
        }

        UpdateDefaultWorld(delta_time);
    }

    void World::New()
    {
        Clear();
        CreateDefaultWorld();
    }

    bool World::SaveToFile(const string& filePathIn)
    {
        // Add scene file extension to the filepath if it's missing
        auto file_path = filePathIn;
        if (FileSystem::GetExtensionFromFilePath(file_path) != EXTENSION_WORLD)
        {
            file_path += EXTENSION_WORLD;
        }

        m_name      = FileSystem::GetFileNameWithoutExtensionFromFilePath(file_path);
        m_file_path = file_path;

        // Notify subsystems that need to save data
        SP_FIRE_EVENT(EventType::WorldSaveStart);

        // Create a prefab file
        auto file = make_unique<FileStream>(file_path, FileStream_Write);
        if (!file->IsOpen())
        {
            LOG_ERROR("Failed to open file.");
            return false;
        }

        // Only save root entities as they will also save their descendants
        vector<shared_ptr<Entity>> root_actors = EntityGetRoots();
        const uint32_t root_entity_count = static_cast<uint32_t>(root_actors.size());

        // Start progress tracking and timing
        const Stopwatch timer;
        ProgressTracker::GetProgress(ProgressType::World).Start(root_entity_count, "Saving world...");

        // Save root entity count
        file->Write(root_entity_count);

        // Save root entity IDs
        for (shared_ptr<Entity>& root : root_actors)
        {
            file->Write(root->GetObjectId());
        }

        // Save root entities
        for (shared_ptr<Entity>& root : root_actors)
        {
            root->Serialize(file.get());
            ProgressTracker::GetProgress(ProgressType::World).JobDone();
        }

        // Report time
        LOG_INFO("World \"%s\" has been saved. Duration %.2f ms", m_file_path.c_str(), timer.GetElapsedTimeMs());

        // Notify subsystems waiting for us to finish
        SP_FIRE_EVENT(EventType::WorldSavedEnd);

        return true;
    }

    bool World::LoadFromFile(const string& file_path)
    {
        if (!FileSystem::Exists(file_path))
        {
            LOG_ERROR("\"%s\" was not found.", file_path.c_str());
            return false;
        }

        // Open file
        unique_ptr<FileStream> file = make_unique<FileStream>(file_path, FileStream_Read);
        if (!file->IsOpen())
        {
            LOG_ERROR("Failed to open \"%s\"", file_path.c_str());
            return false;
        }

        // Clear current entities
        Clear();

        m_name      = FileSystem::GetFileNameWithoutExtensionFromFilePath(file_path);
        m_file_path = file_path;

        // Notify subsystems that need to load data
        SP_FIRE_EVENT(EventType::WorldLoadStart);

        // Load root entity count
        const uint32_t root_entity_count = file->ReadAs<uint32_t>();

        // Start progress tracking and timing
        ProgressTracker::GetProgress(ProgressType::World).Start(root_entity_count, "Loading world...");
        const Stopwatch timer;

        // Load root entity IDs
        for (uint32_t i = 0; i < root_entity_count; i++)
        {
            shared_ptr<Entity> entity = EntityCreate();
            entity->SetObjectId(file->ReadAs<uint64_t>());
        }

        // Serialize root entities
        for (uint32_t i = 0; i < root_entity_count; i++)
        {
            m_entities[i]->Deserialize(file.get(), nullptr);
            ProgressTracker::GetProgress(ProgressType::World).JobDone();
        }

        // Report time
        LOG_INFO("World \"%s\" has been loaded. Duration %.2f ms", m_file_path.c_str(), timer.GetElapsedTimeMs());

        SP_FIRE_EVENT(EventType::WorldLoadEnd);

        return true;
    }

    bool World::IsLoading()
    {
        const bool is_loading_model = ProgressTracker::GetProgress(ProgressType::ModelImporter).IsLoading();
        const bool is_loading_scene = ProgressTracker::GetProgress(ProgressType::World).IsLoading();
        return is_loading_model || is_loading_scene;
    }

    shared_ptr<Entity> World::EntityCreate(bool is_active /*= true*/)
    {
        lock_guard lock(m_mutex_create_entity);

        shared_ptr<Entity> entity = m_entities.emplace_back(make_shared<Entity>(m_context));
        entity->SetActive(is_active);
        return entity;
    }

    bool World::EntityExists(const shared_ptr<Entity>& entity)
    {
        if (!entity)
            return false;

        return EntityGetById(entity->GetObjectId()) != nullptr;
    }

    void World::EntityRemove(const shared_ptr<Entity>& entity)
    {
        if (!entity)
            return;

        // Mark for destruction but don't delete now
        // as the Renderer might still be using it.
        entity->MarkForDestruction();
        m_resolve = true;
    }

    vector<shared_ptr<Entity>> World::EntityGetRoots()
    {
        vector<shared_ptr<Entity>> root_entities;
        for (const shared_ptr<Entity>& entity : m_entities)
        {
            if (entity->GetTransform()->IsRoot())
            {
                root_entities.emplace_back(entity);
            }
        }

        return root_entities;
    }

    const shared_ptr<Entity>& World::EntityGetByName(const string& name)
    {
        for (const auto& entity : m_entities)
        {
            if (entity->GetObjectName() == name)
                return entity;
        }

        static shared_ptr<Entity> empty;
        return empty;
    }

    const shared_ptr<Entity>& World::EntityGetById(const uint64_t id)
    {
        for (const auto& entity : m_entities)
        {
            if (entity->GetObjectId() == id)
                return entity;
        }

        static shared_ptr<Entity> empty;
        return empty;
    }

    void World::Clear()
    {
        // Notify subsystems that need to flush (like the Renderer)
        SP_FIRE_EVENT(EventType::WorldPreClear);

        // Notify any systems that need to clear (like the ResourceCache)
        SP_FIRE_EVENT(EventType::WorldClear);

        // Clear the entities
        m_entities.clear();

        m_name.clear();
        m_file_path.clear();

        m_resolve = true;
    }

    // Removes an entity and all of it's children
    void World::_EntityRemove(const shared_ptr<Entity>& entity)
    {
        // Remove any descendants
        auto children = entity->GetTransform()->GetChildren();
        for (const auto& child : children)
        {
            EntityRemove(child->GetEntity()->GetPtrShared());
        }

        // Keep a reference to it's parent (in case it has one)
        auto parent = entity->GetTransform()->GetParent();

        // Remove this entity
        for (auto it = m_entities.begin(); it < m_entities.end();)
        {
            const auto temp = *it;
            if (temp->GetObjectId() == entity->GetObjectId())
            {
                it = m_entities.erase(it);
                break;
            }
            ++it;
        }

        // If there was a parent, update it
        if (parent)
        {
            parent->AcquireChildren();
        }
    }

    void World::CreateDefaultWorld()
    {
        // Asset directories
        ResourceCache* resource_cache = m_context->GetSubsystem<ResourceCache>();

        // Environment
        {
            shared_ptr<Entity> environment = EntityCreate();
            environment->SetName("environment");
            environment->AddComponent<Environment>();
        }

        // Camera
        {
            shared_ptr<Entity> entity = EntityCreate();
            entity->SetName("camera");

            entity->AddComponent<Camera>();
            entity->AddComponent<AudioListener>();
            entity->GetTransform()->SetPosition(Vector3(1.0f, 0.7760f, -1.5f));
            entity->GetTransform()->SetRotation(Quaternion::FromEulerAngles(Vector3(8.3995f, -37.5985f, 0.0f)));
        }

        // Light - Directional
        {
            shared_ptr<Entity> entity = EntityCreate();
            entity->SetName("directional_light");

            entity->GetTransform()->SetRotation(Quaternion::FromEulerAngles(30.0f, 30.0, 0.0f));
            entity->GetTransform()->SetPosition(Vector3(0.0f, 10.0f, 0.0f));

            Light* light = entity->AddComponent<Light>();
            light->SetLightType(LightType::Directional);
        }

        // 3D model - Helmet
        {
            m_default_world_model = make_unique<Model>(m_context);
            if (m_default_world_model->LoadFromFile("project\\models\\damaged_helmet\\DamagedHelmet.gltf"))
            {
                Entity* entity = m_default_world_model->GetRootEntity();
                entity->SetName("helmet");

                entity->GetTransform()->SetPosition(Vector3(0.0f, 0.6f, 0.0f));
            }
        }

        // 3d model - Cube
        {
            shared_ptr<Entity> entity = EntityCreate();
            entity->SetName("cube");

            entity->GetTransform()->SetPosition(Vector3(0.0, 2.0f, 4.0f));

            // Add a default material and a default cube geometry
            Renderable* renderable = entity->AddComponent<Renderable>();
            renderable->SetGeometry(DefaultGeometry::Cube);
            renderable->SetDefaultMaterial();

            // Physics - Box
            entity->AddComponent<RigidBody>()->SetMass(1.0f);
            entity->AddComponent<Collider>()->SetShapeType(ColliderShape::Box);
        }

        // 3D model - floor
        {
            shared_ptr<Entity> entity = EntityCreate();
            entity->SetName("floor");

            // Position at a high of 0.01 to avoid z-fighting with the world grid
            entity->GetTransform()->SetPosition(Vector3(0.0f, 0.01f, 0.0f));
            entity->GetTransform()->SetScale(Vector3(20.0f, 0.0f, 20.0f));

            Renderable* renderable = entity->AddComponent<Renderable>();
            renderable->SetGeometry(DefaultGeometry::Quad);
            renderable->SetDefaultMaterial();

            // Physics - Static plane
            entity->AddComponent<RigidBody>();
            entity->AddComponent<Collider>()->SetShapeType(ColliderShape::StaticPlane);
        }

        // Music
        {
            shared_ptr<Entity> entity = EntityCreate();
            entity->SetName("audio_source");

            AudioSource* audio_source = entity->AddComponent<AudioSource>();
            audio_source->SetAudioClip("project\\music\\kenny_ibizarre_epic_you.mp3");
            audio_source->SetLoop(true);
        }
    }

    void World::UpdateDefaultWorld(double delta_time)
    {
        if (!m_default_world_model || !m_default_world_model->GetRootEntity())
            return;

        Entity* entity = m_default_world_model->GetRootEntity();

        // If a default model exists, slowly rotate it
        entity->GetTransform()->Rotate(Quaternion::FromEulerAngles(0.0f, 0.0f, -5.0f * static_cast<float>(delta_time)));

        // Play!
        if (!m_default_world_started)
        {
            m_context->m_engine->EngineMode_Toggle(Spartan::Engine_Mode::Engine_Game);
            m_default_world_started = true;
        }

        // Slowly rotate the model
        if (m_context->m_engine->EngineMode_IsSet(Engine_Game))
        {
            entity->GetTransform()->Rotate(Quaternion::FromEulerAngles(0.0f, 0.0f, -5.0f * static_cast<float>(delta_time)));
        }
    }
}
