#pragma once

#include "Inline/BasicTypes.h"
#include "Inline/HashMap.h"

// A map that's somewhere between an array and a HashMap.
// It's keyed by a range of integers, but sparsely maps those integers to elements.
template<typename Index, typename Element> struct IndexMap
{
	IndexMap(Index inMinIndex, Index inMaxIndex)
	: lastIndex(inMinIndex - 1), minIndex(inMinIndex), maxIndex(inMaxIndex)
	{
		wavmAssert(maxIndex >= minIndex);
	}

	// Allocates an index, and adds the element to the map. Indices are allocated sequentially,
	// starting at minIndex, and wrapping back to minIndex after maxIndex. After the allocator has
	// wrapped back to previously allocated indices, indices are probed sequentially until an
	// unallocated one is found. Because of this, it takes O(N) time to add an element.
	// If an index couldn't be allocated, returns false. Otherwise, returns true and the index the
	// element was allocated at is written to the outIndex argument.
	template<typename... Args> Index add(Index failIndex, Args&&... args)
	{
		// If all possible indices are allocated, return failure.
		if(map.size() > Uptr(maxIndex - minIndex + 1)) { return failIndex; }

		// Starting from the index after the last index to be allocated, check indices sequentially
		// until one is found that isn't allocated.
		do
		{
			++lastIndex;
			if(lastIndex > maxIndex) { lastIndex = minIndex; }
		} while(map.contains(lastIndex));

		// Add the element to the map with the given index.
		wavmAssert(lastIndex >= minIndex);
		wavmAssert(lastIndex <= maxIndex);
		map.addOrFail(lastIndex, std::forward<Args>(args)...);

		return lastIndex;
	}

	// Inserts an element at a specific index. If the index is already allocated, asserts.
	template<typename... Args> void insertOrFail(Index index, Args&&... args)
	{
		wavmAssert(index >= minIndex);
		wavmAssert(index <= maxIndex);
		map.addOrFail(index, std::forward<Args>(args)...);
	}

	// Removes an element by index. If there wasn't an allocated at the specified index, asserts.
	void removeOrFail(Index index)
	{
		wavmAssert(index >= minIndex);
		wavmAssert(index <= maxIndex);
		map.removeOrFail(index);
	}

	// Returns whether the specified index is allocated.
	bool contains(Index index) const
	{
		wavmAssert(index >= minIndex);
		wavmAssert(index <= maxIndex);
		return map.contains(index);
	}

	// Returns the element bound to the specified index. Behavior is undefined for if the index
	// isn't allocated.
	const Element& operator[](Index index) const
	{
		wavmAssert(index >= minIndex);
		wavmAssert(index <= maxIndex);
		return map[index];
	}

	// Returns the number of allocated index/element pairs.
	Uptr size() const { return map.size(); }

	Index getMinIndex() const { return minIndex; }
	Index getMaxIndex() const { return maxIndex; }

	struct Iterator
	{
		template<typename, typename> friend struct IndexMap;

		bool operator!=(const Iterator& other) { return mapIt != other.mapIt; }
		bool operator==(const Iterator& other) { return mapIt == other.mapIt; }
		operator bool() const { return bool(mapIt); }
		void operator++() { ++mapIt; }

		const Element& operator*() const { return mapIt->value; }
		const Element* operator->() const { return &mapIt->value; }

	private:
		HashMapIterator<Index, Element> mapIt;

		Iterator(HashMapIterator<Index, Element>&& inMapIt) : mapIt(inMapIt) {}
	};

	Iterator begin() const { return Iterator(map.begin()); }
	Iterator end() const { return Iterator(map.end()); }

private:
	Index lastIndex;
	Index minIndex;
	Index maxIndex;
	HashMap<Index, Element> map;
};
