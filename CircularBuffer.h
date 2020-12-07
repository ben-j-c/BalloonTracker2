#pragma once
#include <array>


template<class T, size_t N>
class CircularBuffer {
public:


	T& back() {
		while (nextIndex(insertIndex) == readIndex)
			std::this_thread::yield();
		return buffer[insertIndex];
	}

	void push_back() {
		while (nextIndex(insertIndex) == readIndex)
			std::this_thread::yield();
		insertIndex = nextIndex(insertIndex);
	}

	T& front() {
		while (insertIndex == readIndex)
			std::this_thread::yield();
		return buffer[readIndex];
	}

	void pop_front() {
		while (insertIndex == readIndex)
			std::this_thread::yield();
		readIndex = nextIndex(readIndex);
	}

	size_t size() {
		if (insertIndex < readIndex) {
			return insertIndex + N - readIndex;
		}
		return insertIndex - readIndex;
	}

	size_t capacity() const { return N; };

private:
	size_t insertIndex;
	size_t readIndex;
	std::array<T, N> buffer;

	inline size_t nextIndex(size_t idx) const {
		return (idx + 1) % N;
	}
};

