/**
 * petri_net.c - Implementation of Petri Nets
 */

#include "petri_net.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/**
 * Create a new Petri net
 */
PetriNet* petri_net_create(const char* name) {
    PetriNet* net = (PetriNet*)malloc(sizeof(PetriNet));
    if (!net) {
        return NULL;
    }
    
    memset(net, 0, sizeof(PetriNet));
    strncpy(net->name, name, MAX_NAME_LENGTH - 1);
    net->name[MAX_NAME_LENGTH - 1] = '\0';
    
    return net;
}

/**
 * Free all resources associated with a Petri net
 */
void petri_net_destroy(PetriNet* net) {
    if (net) {
        free(net);
    }
}

/**
 * Add a place to the Petri net
 */
int petri_net_add_place(PetriNet* net, 
                        const char* name, 
                        uint32_t initial_tokens, 
                        uint32_t capacity) {
    if (!net || net->place_count >= MAX_PLACES) {
        return -1;
    }
    
    Place* place = &net->places[net->place_count];
    
    // Initialize place
    strncpy(place->name, name, MAX_NAME_LENGTH - 1);
    place->name[MAX_NAME_LENGTH - 1] = '\0';
    place->id = net->place_count;
    place->tokens = initial_tokens;
    place->capacity = capacity;
    
    // If capacity is defined and initial tokens exceed it, adjust
    if (capacity > 0 && initial_tokens > capacity) {
        place->tokens = capacity;
    }
    
    return net->place_count++;
}

/**
 * Add a transition to the Petri net
 */
int petri_net_add_transition(PetriNet* net, 
                             const char* name,
                             bool (*condition)(Transition*, void*),
                             void (*action)(Transition*, void*)) {
    if (!net || net->transition_count >= MAX_TRANSITIONS) {
        return -1;
    }
    
    Transition* transition = &net->transitions[net->transition_count];
    
    // Initialize transition
    strncpy(transition->name, name, MAX_NAME_LENGTH - 1);
    transition->name[MAX_NAME_LENGTH - 1] = '\0';
    transition->id = net->transition_count;
    transition->enabled = false;
    transition->condition = condition;
    transition->action = action;
    
    return net->transition_count++;
}

/**
 * Add an arc between a place and a transition
 */
int petri_net_add_arc(PetriNet* net, 
                      uint32_t place_id, 
                      uint32_t transition_id,
                      ArcType type, 
                      uint32_t weight) {
    if (!net || net->arc_count >= MAX_ARCS) {
        return -1;
    }
    
    // Validate place and transition IDs
    if (place_id >= net->place_count || transition_id >= net->transition_count) {
        return -1;
    }
    
    // Weight must be positive
    if (weight == 0) {
        return -1;
    }
    
    Arc* arc = &net->arcs[net->arc_count];
    
    // Initialize arc
    arc->id = net->arc_count;
    arc->type = type;
    arc->weight = weight;
    arc->place_id = place_id;
    arc->transition_id = transition_id;
    
    return net->arc_count++;
}

/**
 * Get a place by ID
 */
Place* petri_net_get_place(PetriNet* net, uint32_t place_id) {
    if (!net || place_id >= net->place_count) {
        return NULL;
    }
    
    return &net->places[place_id];
}

/**
 * Get a place by name
 */
Place* petri_net_get_place_by_name(PetriNet* net, const char* name) {
    if (!net || !name) {
        return NULL;
    }
    
    for (uint32_t i = 0; i < net->place_count; i++) {
        if (strcmp(net->places[i].name, name) == 0) {
            return &net->places[i];
        }
    }
    
    return NULL;
}

/**
 * Get a transition by ID
 */
Transition* petri_net_get_transition(PetriNet* net, uint32_t transition_id) {
    if (!net || transition_id >= net->transition_count) {
        return NULL;
    }
    
    return &net->transitions[transition_id];
}

/**
 * Get a transition by name
 */
Transition* petri_net_get_transition_by_name(PetriNet* net, const char* name) {
    if (!net || !name) {
        return NULL;
    }
    
    for (uint32_t i = 0; i < net->transition_count; i++) {
        if (strcmp(net->transitions[i].name, name) == 0) {
            return &net->transitions[i];
        }
    }
    
    return NULL;
}

/**
 * Check if a specific transition is enabled
 */
bool petri_net_is_transition_enabled(PetriNet* net, uint32_t transition_id) {
    if (!net || transition_id >= net->transition_count) {
        return false;
    }
    
    Transition* transition = &net->transitions[transition_id];
    
    // Check input arcs: all input places must have enough tokens
    for (uint32_t i = 0; i < net->arc_count; i++) {
        Arc* arc = &net->arcs[i];
        
        if (arc->transition_id == transition_id) {
            Place* place = &net->places[arc->place_id];
            
            if (arc->type == ARC_INPUT) {
                // Regular input arc: need enough tokens
                if (place->tokens < arc->weight) {
                    return false;
                }
            } else if (arc->type == ARC_INHIBIT) {
                // Inhibitor arc: transition is disabled if place has tokens >= weight
                if (place->tokens >= arc->weight) {
                    return false;
                }
            }
        }
    }
    
    // Check additional condition if provided
    if (transition->condition) {
        if (!transition->condition(transition, net->user_data)) {
            return false;
        }
    }
    
    return true;
}

/**
 * Update the enabled status of all transitions
 */
void petri_net_update_enabled_transitions(PetriNet* net) {
    if (!net) {
        return;
    }
    
    for (uint32_t i = 0; i < net->transition_count; i++) {
        net->transitions[i].enabled = petri_net_is_transition_enabled(net, i);
    }
}

/**
 * Fire a specific transition if enabled
 */
bool petri_net_fire_transition(PetriNet* net, uint32_t transition_id) {
    if (!net || transition_id >= net->transition_count) {
        return false;
    }
    
    Transition* transition = &net->transitions[transition_id];
    
    // Check if transition is enabled
    if (!petri_net_is_transition_enabled(net, transition_id)) {
        return false;
    }
    
    // First phase: consume tokens from input places
    for (uint32_t i = 0; i < net->arc_count; i++) {
        Arc* arc = &net->arcs[i];
        
        if (arc->transition_id == transition_id && arc->type == ARC_INPUT) {
            Place* place = &net->places[arc->place_id];
            place->tokens -= arc->weight;
        }
    }
    
    // Second phase: produce tokens in output places
    for (uint32_t i = 0; i < net->arc_count; i++) {
        Arc* arc = &net->arcs[i];
        
        if (arc->transition_id == transition_id && arc->type == ARC_OUTPUT) {
            Place* place = &net->places[arc->place_id];
            
            // Respect capacity constraints
            if (place->capacity > 0) {
                uint32_t new_tokens = place->tokens + arc->weight;
                if (new_tokens > place->capacity) {
                    new_tokens = place->capacity;
                }
                place->tokens = new_tokens;
            } else {
                place->tokens += arc->weight;
            }
        }
    }
    
    // Execute action if provided
    if (transition->action) {
        transition->action(transition, net->user_data);
    }
    
    // Update enabled status of all transitions
    petri_net_update_enabled_transitions(net);
    
    return true;
}

/**
 * Fire all enabled transitions in the Petri net
 */
int petri_net_fire_enabled_transitions(PetriNet* net) {
    if (!net) {
        return 0;
    }
    
    int fired_count = 0;
    
    // Update enabled status
    petri_net_update_enabled_transitions(net);
    
    // Attempt to fire each transition
    for (uint32_t i = 0; i < net->transition_count; i++) {
        if (net->transitions[i].enabled) {
            if (petri_net_fire_transition(net, i)) {
                fired_count++;
            }
        }
    }
    
    return fired_count;
}

/**
 * Get the current marking (token distribution) of the Petri net
 */
void petri_net_get_marking(PetriNet* net, uint32_t* marking) {
    if (!net || !marking) {
        return;
    }
    
    for (uint32_t i = 0; i < net->place_count; i++) {
        marking[i] = net->places[i].tokens;
    }
}

/**
 * Set the marking (token distribution) of the Petri net
 */
bool petri_net_set_marking(PetriNet* net, const uint32_t* marking) {
    if (!net || !marking) {
        return false;
    }
    
    // Check capacity constraints
    for (uint32_t i = 0; i < net->place_count; i++) {
        if (net->places[i].capacity > 0 && marking[i] > net->places[i].capacity) {
            return false;
        }
    }
    
    // Set tokens
    for (uint32_t i = 0; i < net->place_count; i++) {
        net->places[i].tokens = marking[i];
    }
    
    // Update enabled transitions
    petri_net_update_enabled_transitions(net);
    
    return true;
}

/**
 * Print the current state of the Petri net
 */
void petri_net_print(PetriNet* net) {
    if (!net) {
        return;
    }
    
    printf("Petri Net: %s\n", net->name);
    printf("Places (%u):\n", net->place_count);
    for (uint32_t i = 0; i < net->place_count; i++) {
        Place* place = &net->places[i];
        printf("  P%u (%s): %u token(s)", place->id, place->name, place->tokens);
        if (place->capacity > 0) {
            printf(" (capacity: %u)", place->capacity);
        }
        printf("\n");
    }
    
    printf("Transitions (%u):\n", net->transition_count);
    for (uint32_t i = 0; i < net->transition_count; i++) {
        Transition* transition = &net->transitions[i];
        printf("  T%u (%s): %s\n", 
               transition->id, 
               transition->name, 
               transition->enabled ? "enabled" : "disabled");
    }
    
    printf("Arcs (%u):\n", net->arc_count);
    for (uint32_t i = 0; i < net->arc_count; i++) {
        Arc* arc = &net->arcs[i];
        const char* arc_type_str = "unknown";
        
        switch (arc->type) {
            case ARC_INPUT:
                arc_type_str = "input";
                printf("  A%u: P%u -> T%u (weight: %u)\n", 
                       arc->id, arc->place_id, arc->transition_id, arc->weight);
                break;
            case ARC_OUTPUT:
                arc_type_str = "output";
                printf("  A%u: T%u -> P%u (weight: %u)\n", 
                       arc->id, arc->transition_id, arc->place_id, arc->weight);
                break;
            case ARC_INHIBIT:
                arc_type_str = "inhibit";
                printf("  A%u: P%u -o T%u (weight: %u)\n", 
                       arc->id, arc->place_id, arc->transition_id, arc->weight);
                break;
        }
    }
    printf("\n");
}

/**
 * Run a simulation of the Petri net for a specified number of steps
 */
int petri_net_simulate(PetriNet* net, int steps, bool print_steps) {
    if (!net || steps <= 0) {
        return 0;
    }
    
    int total_fired = 0;
    
    for (int i = 0; i < steps; i++) {
        if (print_steps) {
            printf("Step %d:\n", i + 1);
            petri_net_print(net);
        }
        
        int fired = petri_net_fire_enabled_transitions(net);
        total_fired += fired;
        
        if (fired == 0) {
            if (print_steps) {
                printf("No transitions fired - simulation halted (deadlock).\n");
            }
            break;
        }
    }
    
    return total_fired;
}

/**
 * Check if the Petri net is in a deadlock state (no enabled transitions)
 */
bool petri_net_is_deadlocked(PetriNet* net) {
    if (!net) {
        return true;
    }
    
    petri_net_update_enabled_transitions(net);
    
    for (uint32_t i = 0; i < net->transition_count; i++) {
        if (net->transitions[i].enabled) {
            return false;
        }
    }
    
    return true;
}