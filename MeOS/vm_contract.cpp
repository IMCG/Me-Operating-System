#include "vm_contract.h"

void vm_contract_init(vm_contract* c, uint32 low_addr, uint32 high_addr)
{
	printfln("high addr: %h, low addr: %h", high_addr, low_addr);
	// assert page align
	if (high_addr % vmmngr_get_page_size() != 0 || low_addr % vmmngr_get_page_size() != 0 || low_addr >= high_addr)
		DEBUG("high or low process address space is not page aligned or fundamental address inequality does not hold");

	c->highest_addr = high_addr;
	c->lowest_addr = low_addr;

	ordered_vector_init(&c->contract);

	// set guard non removable areas
	vm_area area;
	vm_area_init(&area);
	area.flags = VM_AREA_NON_REMOVE;

	vm_area_set_bounds(&area, low_addr, 4096);
	ordered_vector_insert(&c->contract, area);

	vm_area_set_bounds(&area, high_addr - 4096, 4096);
	ordered_vector_insert(&c->contract, area);
}

bool vm_contract_add_area(vm_contract* c, vm_area* new_area)
{
	// sanity check
	if (vm_area_is_ok(new_area) == false)
		return false;

	// assert new area is validly inside our process space
	if (new_area->start_addr < c->lowest_addr || vm_area_get_end_address(new_area) >= c->highest_addr)
		return false;

	if (c->contract.count == 0)		// on zero count insert immediatelly. This should never happen as of init
	{
		PANIC("Heavy problem");		// in release mode can be ommited
		ordered_vector_insert(&c->contract, *new_area);
		return true;
	}

	// crazy binary search to get smallest closest starting address
	uint32 first = 0, last = c->contract.count - 1;
	uint32 mid;

	uint32 target = new_area->start_addr;
	bool found = false;

	while (first <= last)
	{
		mid = (first + last) / 2;

		if (c->contract.data[mid].start_addr < target)
			first = mid + 1;
		else if (c->contract.data[mid].start_addr > target)
			last = mid - 1;
		else
		{
			found = true;
			break;
		}
	}

	if (found)		// found another area with the SAME start address so definitely fail
		return false;

	if (last + 1 == c->contract.count)		// this means we found the last-guard area
		PANIC("SOMETHING IS REALLY WRONG");

	// do intersection check. Consider first > last as found == false => data[last] points to the smallest start address
	if (vm_area_intersects(&c->contract.data[last], new_area) == false &&
		vm_area_intersects(&c->contract.data[last + 1], new_area) == false)
	{
		ordered_vector_insert(&c->contract, *new_area);
		return true;
	}

	printfln("FAIL AT: %h", target);
	return false;
}

void vm_contract_remove_area(vm_contract* c, vm_area* area)
{
	if (vm_area_is_removable(area) == false)
	{
		DEBUG("Attempting to remove non-removable vm_area");
		return;
	}

	ordered_vector_remove(&c->contract, ordered_vector_find(&c->contract, *area));
}

bool vm_contract_expand_area(vm_contract* c, vm_area* area, uint32 length)
{
	if (area == 0 || c == 0 || vm_area_is_removable(area) == false)
		return false;

	if (length % vmmngr_get_page_size() != 0)
	{
		PANIC("Area expansion failed due to unaligned length");		// in release mode can be ommited
		return false;
	}

	uint32 index = ordered_vector_find(&c->contract, *area);
	if (index >= c->contract.count)		// area was not found
		return false;

	// no need to test index - 1 and index + 1 validity as both 0 and count - 1 areas are non-removable. See above if
	vm_area* adjacent;
	vm_area temp = *area;

	if (vm_area_grows_down(area))
	{
		adjacent = &c->contract.data[index - 1];
		temp.start_addr -= length;
	}
	else
	{
		adjacent = &c->contract.data[index + 1];
		temp.end_addr += length;
	}

	// sanity check before altering stucture data
	if (vm_area_is_ok(&temp) == false)
		return false;

	if (vm_area_intersects(adjacent, &temp))
		return false;			// on intersection leave the original area unaffected and fail

	*area = temp;				// expansion is possible so do it

	return true;
}

vm_area* vm_contract_find_area(vm_contract* c, uint32 address)
{
	if (c == 0 || c->contract.count == 0 || address < c->lowest_addr || address >= c->highest_addr)	// this should never happen
		return 0;

	uint32 first = 0, last = c->contract.count - 1, mid;
	bool found = false;

	while (first <= last)
	{
		mid = (first + last) / 2;

		if (c->contract.data[mid].start_addr < address)
			first = mid + 1;
		else if (c->contract.data[mid].start_addr > address)
			last = mid - 1;
		else
		{
			found = true;
			break;
		}
	}

	if (found)
		return &c->contract.data[mid];

	if (address <= vm_area_get_end_address(&c->contract.data[last]))
		return &c->contract.data[last];

	return 0;
}

uint32 vm_contract_get_area_for_length(vm_contract* c, uint32 length)
{
	if (c == 0 || c->contract.count == 0)
		return 0;

	for (uint32 i = 0; i < c->contract.count - 1; i++)
	{
		// test two successive area for in-between empty length
		uint32 start = vm_area_get_start_address(&c->contract.data[i + 1]);
		uint32 end = vm_area_get_end_address(&c->contract.data[i]) + 1;

		if (start - end >= length)
			return end;
	}

	return 0;
}

void vm_contract_print(vm_contract* c)
{
	for (int i = 0; i < c->contract.count; i++)
		vm_area_print(&c->contract.data[i]);
}