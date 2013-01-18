/*******************************************************************************
 * DANIEL'S ALGORITHM IMPLEMENTAIONS
 *
 *  /\  |  _   _  ._ o _|_ |_  ._ _   _ 
 * /--\ | (_| (_) |  |  |_ | | | | | _> 
 *         _|                      
 *
 * PERFECT HASH
 * 
 * http://en.wikipedia.org/wiki/Perfect_hash 
 *
 ******************************************************************************/
#ifndef __PERFECT_HASH_H__
#define __PERFECT_HASH_H__
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <generic.h>
#include <universal_hash.h>
#include <stack.h>

struct PerfSlotL2 {
	uint32_t cnt; // collison count
	void * value;	
};

struct PerfSlotL1 {
	uint32_t cnt; // collison count
	struct uhash_params params;   // 2nd level 
	struct PerfSlotL2 * lv2_slots;
	void * value;	
} PerfSlotL1;

struct PerfHT {
	struct PerfSlotL1 * slots;
	struct uhash_params params;   // 1st level 
	uint32_t num_slots;
};

/**
 * init level-2 slots with known collides 
 * the routine will find another hash function never collides again!!
 */
inline static void perfect_hash_lv2_slot_init(struct PerfSlotL1 * lv1_slot, Stack * collides)
{
// init another hash function & 2nd level 
	uint32_t bucket = lv1_slot->cnt * lv1_slot->cnt;
	
	uhash_init(&lv1_slot->params, bucket);
	struct PerfSlotL2 * lv2_slots = (struct PerfSlotL2 *) malloc(sizeof(struct PerfSlotL2) * lv1_slot->params.prime);
	lv1_slot->lv2_slots = lv2_slots;
	
	Stack *S2 = createStack(lv1_slot->cnt);

retry:
	memset(lv2_slots,0,sizeof(struct PerfSlotL2) * lv1_slot->params.prime);
	uhash_init(&lv1_slot->params, bucket);

	// check collide	
	uint32_t key;
	int j;

	for(j=0; j<lv1_slot->cnt;j++) {
		key = (uint32_t)top(collides);push(S2,(void *)key);
		pop(collides);
		int hash = uhash_integer(&lv1_slot->params, key);
		
		if (++lv2_slots[hash].cnt > 1) {
			// collide again!!! rollback stack ops
			int r;
			for(r=j;r>=0;r--) {
				key =(uint32_t)top(S2);push(collides,(void *)key);
				pop(S2);
			}
			goto retry;
		}
	}
	destroyStack(S2);
}

/**
 * level-2 hash pre-work
 * collect collides for each level-1 slots
 */
inline static void perfect_hash_lv2_init(struct PerfHT * ht, uint32_t keys[], int len)
{
	// stacks for temporary storing keys
	Stack *S1 = createStack(len);

	struct PerfSlotL1 * slots = ht->slots;
	int i;

	for (i = 0; i < ht->params.prime; i++) {
		if (slots[i].cnt > 1) {	
			int j;
			
			// find collide keys
			for(j=0;j<len;j++) {
				if(uhash_integer(&ht->params, keys[j]) == i) {
					push(S1, (void *)keys[j]);
				}
			}
			// 
			perfect_hash_lv2_slot_init(&slots[i], S1); 
		}
	}
	destroyStack(S1);
}

/**
 * init a perfect hash table, all keys should be provided first
 */
inline struct PerfHT * perfect_hash_init(uint32_t keys[],int len)
{
	struct PerfHT * ht = (struct PerfHT *)malloc(sizeof(struct PerfHT));

	// remove duplicate keys
	len = remove_dup(keys, len);

	// 1-level hashing 
	uhash_init(&ht->params, len);
	
	struct PerfSlotL1 * slots = 
			(struct PerfSlotL1 *) malloc(sizeof(struct PerfSlotL1)*ht->params.prime);
	memset(slots,0, sizeof(struct PerfSlotL1) * ht->params.prime);

	ht->slots = slots;

	int i;
	for (i = 0; i < len; i++) {
		uint32_t hash = uhash_integer(&ht->params, keys[i]);
		slots[hash].cnt++;
	}

	for (i = 0; i < ht->params.prime; i++) {
		// printf("slot %d: cnt %d\n", i, slots[i].cnt);
	}

	// 2-level processing
	perfect_hash_lv2_init(ht, keys, len);

	return ht;
}

/**
 * destroy the hash table
 */
inline void perfect_hash_destroy(struct PerfHT * ht)
{
	int i;
	for (i=0; i<ht->params.prime; i++) {
		struct PerfSlotL1 * slot = &ht->slots[i];
		if(slot->cnt > 1) free(slot->lv2_slots);
	}
	free(ht->slots);
	free(ht);
}

/**
 * set a key->value pair in the table
 */
inline void perfect_hash_set(struct PerfHT * ht, uint32_t key, void * value)
{
	uint32_t hash;
	hash  = uhash_integer(&ht->params, key);
	if (ht->slots[hash].cnt == 1) {
		ht->slots[hash].value = value; // put value to slot
		return;
	}

	if (ht->slots[hash].cnt >  1) {
		struct PerfSlotL1 * slot = &ht->slots[hash];
		uint32_t hash2 = uhash_integer(&slot->params, key);
		
		// 2nd-level available
		if (slot->lv2_slots[hash2].cnt == 1) {
			slot->lv2_slots[hash2].value = value;
			return;
		}
	}
}

inline void * perfect_hash_get(const struct PerfHT * ht, uint32_t key)
{
	uint32_t hash;
	hash  = uhash_integer(&ht->params, key);
	if (ht->slots[hash].cnt == 1) {
		return ht->slots[hash].value;
	}

	if (ht->slots[hash].cnt >  1) {
		struct PerfSlotL1 * slot = &ht->slots[hash];
		uint32_t hash2 = uhash_integer(&slot->params, key);
		
		// 2nd-level available
		if (slot->lv2_slots[hash2].cnt == 1) {
			return slot->lv2_slots[hash2].value;
		}
	}
	
	return NULL;
}

#endif //