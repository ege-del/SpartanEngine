/*
Copyright(c) 2016-2020 Panos Karabelas

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

#pragma once

//= INCLUDES ==================
#include <vector>
#include <thread>
#include <mutex>
#include <deque>
#include <map>
#include <functional>
#include "../Logging/Log.h"
#include "../Core/ISubsystem.h"
//=============================

namespace Spartan
{
	class Task
	{
	public:
		typedef std::function<void()> function_type;

		Task(function_type&& function)  { m_function = std::forward<function_type>(function); }
        void Execute()                  { m_is_executing = true; m_function(); m_is_executing = false; }
        bool IsExecuting()              { return m_is_executing; }

	private:
        bool m_is_executing = false;
		function_type m_function;
	};

	class Threading : public ISubsystem
	{
	public:
		Threading(Context* context);
		~Threading();

		// This function is invoked by the threads
		void Invoke();

		// Add a task
		template <typename Function>
		void AddTask(Function&& function)
		{
			if (m_threads.empty())
			{
				LOG_WARNING("Threading::AddTask: No available threads, function will execute in the same thread");
				function();
				return;
			}

			// Lock tasks mutex
			std::unique_lock<std::mutex> lock(m_mutex_tasks);

			// Save the task
			m_tasks.push_back(std::make_shared<Task>(std::bind(std::forward<Function>(function))));

			// Unlock the mutex
			lock.unlock();

			// Wake up a thread
			m_condition_var.notify_one();
		}

        template <typename Function>
        void Loop(Function&& function, uint32_t range)
        {
            uint32_t available_threads  = GetThreadsAvailable();
            vector<bool> tasks_done     = vector<bool>(available_threads, false);
            uint32_t task_count         = available_threads + 1; // plus one for the current thread

            uint32_t start  = 0;
            uint32_t end    = 0;
            for (uint32_t i = 0; i < available_threads; i++)
            {
                start   = (range / task_count) * i;
                end     = start + (range / task_count);

                // Kick off task
                AddTask([&function, &tasks_done, i, start, end] { function(start, end); tasks_done[i] = true; });
            }

            // Do last task in the current thread
            function(end, range);

            // Wait till the threads are done
            uint32_t tasks = 0;
            while (tasks != tasks_done.size())
            {
                tasks = 0;
                for (const bool job_done : tasks_done)
                {
                    tasks += job_done ? 1 : 0;
                }
            }
        }

        uint32_t GetThreadCount()       { return m_thread_count; }
        uint32_t GetThreadCountMax()    { return m_thread_max; }
        uint32_t GetThreadsAvailable();

	private:
		uint32_t m_thread_count = 0;
        uint32_t m_thread_max   = 0;
		std::vector<std::thread> m_threads;
		std::deque<std::shared_ptr<Task>> m_tasks;
		std::mutex m_mutex_tasks;
		std::condition_variable m_condition_var;
        std::map<std::thread::id, std::string> m_thread_names;
		bool m_stopping;
	};
}
