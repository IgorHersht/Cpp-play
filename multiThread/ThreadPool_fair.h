#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
template <typename T, std::uint64_t QueueDepth>
	struct concurrent_bounded_queue {};
private:
	std::queue<T> items;
	std::mutex items_mtx;
	std::counting_semaphore <QueueDepth> items_produced{ 0 };
	std::counting_semaphore <QueueDepth > remaining_space{ QueueDepth }
	void push(std::convertible_to <T> auto&& u)	
	T pop();
public:
	constexpr concurrent_bounded_queue() = default;
	void enqueue(std::convertible_to <T> auto&& u);
	T dequeue();
	std::optional<T> try_dequeue();
};
