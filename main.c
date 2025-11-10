#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>




#include "BENSCHILLIBOWL.h"

// Feel free to play with these numbers! This is a great way to
// test your implementation.
#define BENSCHILLIBOWL_SIZE 100
#define NUM_CUSTOMERS 90
#define NUM_COOKS 10
#define ORDERS_PER_CUSTOMER 3
#define EXPECTED_NUM_ORDERS NUM_CUSTOMERS * ORDERS_PER_CUSTOMER

// Global variable for the restaurant.
BENSCHILLIBOWL *bcb;

//static inline int randi(int max_inclusive) {
  //if (max_inclusive <= 0) return 0;
  //return rand() % (max_inclusive +1);
//}

/**
 * Thread funtion that represents a customer. A customer should:
 *  - allocate space (memory) for an order.
 *  - select a menu item.
 *  - populate the order with their menu item and their customer ID.
 *  - add their order to the restaurant.
 */
void* BENSCHILLIBOWLCustomer(void* tid) {
    int customer_id = (int)(long) tid;
    for (int i = 0; i < ORDERS_PER_CUSTOMER; i++) {
      Order* o = (Order*)malloc(sizeof(Order));
      o->menu_item   = PickRandomMenuItem();
      o->customer_id = customer_id;
      o->next        = NULL;
      AddOrder(bcb, o);
      
    }
	return NULL;
}

/**
 * Thread function that represents a cook in the restaurant. A cook should:
 *  - get an order from the restaurant.
 *  - if the order is valid, it should fulfill the order, and then
 *    free the space taken by the order.
 * The cook should take orders from the restaurants until it does not
 * receive an order.
 */
void* BENSCHILLIBOWLCook(void* tid) {
    int cook_id = (int)(long) tid;
	  int orders_fulfilled = 0;
    for(;;) {
      Order* o = GetOrder(bcb);
      if (!o) break;
      pthread_mutex_lock(&bcb->mutex);
      bcb->orders_handled++;
      pthread_mutex_unlock(&bcb->mutex);

      free(o);
      orders_fulfilled++;


    }
	printf("Cook #%d fulfilled %d orders\n", cook_id, orders_fulfilled);
	return NULL;
}

/**
 * Runs when the program begins executing. This program should:
 *  - open the restaurant
 *  - create customers and cooks
 *  - wait for all customers and cooks to be done
 *  - close the restaurant.
 */
int main() {
  srand((unsigned)time(NULL));

  bcb = OpenRestaurant(BENSCHILLIBOWL_SIZE, EXPECTED_NUM_ORDERS);

  pthread_t customers[NUM_CUSTOMERS];
  pthread_t cooks[NUM_COOKS];

  for (intptr_t i = 0; i < NUM_CUSTOMERS; i++) {
    pthread_create(&customers[i], NULL, BENSCHILLIBOWLCustomer, (void*)i);

  }
  for (intptr_t j = 0; j < NUM_COOKS; j++) {
        pthread_create(&cooks[j], NULL, BENSCHILLIBOWLCook, (void*)j);
  }

  for (int i = 0; i < NUM_CUSTOMERS; i++) pthread_join(customers[i], NULL);
  pthread_mutex_lock(&bcb->mutex);
  pthread_cond_broadcast(&bcb->can_get_orders);
  pthread_mutex_unlock(&bcb->mutex);
  
  
  for (int j = 0; j < NUM_COOKS; j++)     pthread_join(cooks[j], NULL);

  CloseRestaurant(bcb);
  return 0;
}