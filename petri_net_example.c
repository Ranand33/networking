#include "petri_net.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*******************************************************************************
 * Petri Net Example: Producer-Consumer System
 *******************************************************************************/

/* Callback functions for transitions */
bool can_produce(Transition* transition, void* user_data) {
    /* Additional logic could be added here */
    return true;
}

void produce_item(Transition* transition, void* user_data) {
    printf("Producer: Item produced\n");
}

bool can_consume(Transition* transition, void* user_data) {
    /* Additional logic could be added here */
    return true;
}

void consume_item(Transition* transition, void* user_data) {
    printf("Consumer: Item consumed\n");
}

void petri_net_producer_consumer_example() {
    printf("\n=== Petri Net Example: Producer-Consumer ===\n\n");
    
    PetriNet* net = petri_net_create("Producer-Consumer System");
    
    /* Add places */
    int producer_ready_id = petri_net_add_place(net, "ProducerReady", 1, 1);
    int buffer_id = petri_net_add_place(net, "Buffer", 0, 5);
    int buffer_space_id = petri_net_add_place(net, "BufferSpace", 5, 5);
    int consumer_ready_id = petri_net_add_place(net, "ConsumerReady", 1, 1);
    
    /* Add transitions */
    int produce_id = petri_net_add_transition(net, "Produce", can_produce, produce_item);
    int consume_id = petri_net_add_transition(net, "Consume", can_consume, consume_item);
    
    /* Add arcs for Produce transition */
    petri_net_add_arc(net, producer_ready_id, produce_id, ARC_INPUT, 1);
    petri_net_add_arc(net, buffer_space_id, produce_id, ARC_INPUT, 1);
    petri_net_add_arc(net, producer_ready_id, produce_id, ARC_OUTPUT, 1);
    petri_net_add_arc(net, buffer_id, produce_id, ARC_OUTPUT, 1);
    
    /* Add arcs for Consume transition */
    petri_net_add_arc(net, consumer_ready_id, consume_id, ARC_INPUT, 1);
    petri_net_add_arc(net, buffer_id, consume_id, ARC_INPUT, 1);
    petri_net_add_arc(net, consumer_ready_id, consume_id, ARC_OUTPUT, 1);
    petri_net_add_arc(net, buffer_space_id, consume_id, ARC_OUTPUT, 1);
    
    /* Print initial state */
    printf("Initial state:\n");
    petri_net_print(net);
    
    /* Simulate the net */
    printf("\nSimulating for 10 steps...\n\n");
    petri_net_simulate(net, 10, true);
    
    /* Clean up */
    petri_net_destroy(net);
}

/*******************************************************************************
 * Petri Net Example: Resource Allocation System
 *******************************************************************************/

void petri_net_resource_allocation_example() {
    printf("\n=== Petri Net Example: Resource Allocation ===\n\n");
    
    PetriNet* net = petri_net_create("Resource Allocation System");
    
    /* Add places */
    int process1_idle_id = petri_net_add_place(net, "Process1Idle", 1, 1);
    int process1_running_id = petri_net_add_place(net, "Process1Running", 0, 1);
    int process2_idle_id = petri_net_add_place(net, "Process2Idle", 1, 1);
    int process2_running_id = petri_net_add_place(net, "Process2Running", 0, 1);
    int resource_a_id = petri_net_add_place(net, "ResourceA", 1, 1);
    int resource_b_id = petri_net_add_place(net, "ResourceB", 1, 1);
    
    /* Add transitions */
    int start_p1_id = petri_net_add_transition(net, "StartP1", NULL, NULL);
    int end_p1_id = petri_net_add_transition(net, "EndP1", NULL, NULL);
    int start_p2_id = petri_net_add_transition(net, "StartP2", NULL, NULL);
    int end_p2_id = petri_net_add_transition(net, "EndP2", NULL, NULL);
    
    /* Add arcs for Process 1 */
    petri_net_add_arc(net, process1_idle_id, start_p1_id, ARC_INPUT, 1);
    petri_net_add_arc(net, resource_a_id, start_p1_id, ARC_INPUT, 1);
    petri_net_add_arc(net, resource_b_id, start_p1_id, ARC_INPUT, 1);
    petri_net_add_arc(net, process1_running_id, start_p1_id, ARC_OUTPUT, 1);
    
    petri_net_add_arc(net, process1_running_id, end_p1_id, ARC_INPUT, 1);
    petri_net_add_arc(net, process1_idle_id, end_p1_id, ARC_OUTPUT, 1);
    petri_net_add_arc(net, resource_a_id, end_p1_id, ARC_OUTPUT, 1);
    petri_net_add_arc(net, resource_b_id, end_p1_id, ARC_OUTPUT, 1);
    
    /* Add arcs for Process 2 */
    petri_net_add_arc(net, process2_idle_id, start_p2_id, ARC_INPUT, 1);
    petri_net_add_arc(net, resource_a_id, start_p2_id, ARC_INPUT, 1);
    petri_net_add_arc(net, process2_running_id, start_p2_id, ARC_OUTPUT, 1);
    
    petri_net_add_arc(net, process2_running_id, end_p2_id, ARC_INPUT, 1);
    petri_net_add_arc(net, process2_idle_id, end_p2_id, ARC_OUTPUT, 1);
    petri_net_add_arc(net, resource_a_id, end_p2_id, ARC_OUTPUT, 1);
    
    /* Print initial state */
    printf("Initial state:\n");
    petri_net_print(net);
    
    /* Simulate the net */
    printf("\nSimulating for 10 steps...\n\n");
    petri_net_simulate(net, 10, true);
    
    /* Clean up */
    petri_net_destroy(net);
}

int main() {
    printf("==================================================\n");
    printf("   Petri Nets Examples  \n");
    printf("==================================================\n");
    
    /* Run Petri Net examples */
    petri_net_producer_consumer_example();
    petri_net_resource_allocation_example();
    
    printf("\nAll examples completed successfully.\n");
    
    return 0;
}