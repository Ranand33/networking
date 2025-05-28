/**
 * petri_net.h - A C implementation of Petri Nets
 * 
 * This header defines the data structures and functions
 * necessary to create and simulate Petri nets.
 */

#ifndef PETRI_NET_H
#define PETRI_NET_H

#include <stdint.h>
#include <stdbool.h>

/* Maximum number of places and transitions in a single net */
#define MAX_PLACES 100
#define MAX_TRANSITIONS 100
#define MAX_ARCS 500
#define MAX_NAME_LENGTH 64

/* Forward declarations */
typedef struct PetriNet PetriNet;
typedef struct Place Place;
typedef struct Transition Transition;
typedef struct Arc Arc;

/* Arc types */
typedef enum {
    ARC_INPUT,   /* Place to Transition */
    ARC_OUTPUT,  /* Transition to Place */
    ARC_INHIBIT  /* Inhibitor arc (prevents transition from firing) */
} ArcType;

/* Place structure */
typedef struct Place {
    char name[MAX_NAME_LENGTH];  /* Name of the place */
    uint32_t id;                 /* Unique ID */
    uint32_t tokens;             /* Current number of tokens */
    uint32_t capacity;           /* Maximum capacity (0 for unlimited) */
} Place;

/* Transition structure */
typedef struct Transition {
    char name[MAX_NAME_LENGTH];  /* Name of the transition */
    uint32_t id;                 /* Unique ID */
    bool enabled;                /* Whether the transition is currently enabled */
    
    /* Optional: Function to determine if a transition is enabled beyond token count */
    bool (*condition)(Transition* transition, void* user_data);
    
    /* Optional: Function to execute when transition fires */
    void (*action)(Transition* transition, void* user_data);
} Transition;

/* Arc structure (connects places and transitions) */
typedef struct Arc {
    uint32_t id;        /* Unique ID */
    ArcType type;       /* Type of arc */
    uint32_t weight;    /* Number of tokens consumed/produced */
    
    /* Endpoints */
    union {
        Place* place;
        uint32_t place_id;
    };
    
    union {
        Transition* transition;
        uint32_t transition_id;
    };
} Arc;

/* Petri net structure */
typedef struct PetriNet {
    char name[MAX_NAME_LENGTH];     /* Name of the Petri net */
    
    /* Places */
    Place places[MAX_PLACES];
    uint32_t place_count;
    
    /* Transitions */
    Transition transitions[MAX_TRANSITIONS];
    uint32_t transition_count;
    
    /* Arcs */
    Arc arcs[MAX_ARCS];
    uint32_t arc_count;
    
    /* User data (can be used by condition/action functions) */
    void* user_data;
} PetriNet;

/**
 * Create a new Petri net
 * 
 * @param name The name of the Petri net
 * @return A pointer to the created Petri net
 */
PetriNet* petri_net_create(const char* name);

/**
 * Free all resources associated with a Petri net
 * 
 * @param net The Petri net to destroy
 */
void petri_net_destroy(PetriNet* net);

/**
 * Add a place to the Petri net
 * 
 * @param net The Petri net
 * @param name The name of the place
 * @param initial_tokens Initial number of tokens
 * @param capacity Maximum token capacity (0 for unlimited)
 * @return ID of the created place, or -1 on failure
 */
int petri_net_add_place(PetriNet* net, 
                        const char* name, 
                        uint32_t initial_tokens, 
                        uint32_t capacity);

/**
 * Add a transition to the Petri net
 * 
 * @param net The Petri net
 * @param name The name of the transition
 * @param condition Optional function to check if transition is enabled
 * @param action Optional function to execute when transition fires
 * @return ID of the created transition, or -1 on failure
 */
int petri_net_add_transition(PetriNet* net, 
                             const char* name,
                             bool (*condition)(Transition*, void*),
                             void (*action)(Transition*, void*));

/**
 * Add an arc between a place and a transition
 * 
 * @param net The Petri net
 * @param place_id ID of the place
 * @param transition_id ID of the transition
 * @param type Type of arc (input, output, inhibitor)
 * @param weight Number of tokens consumed/produced
 * @return ID of the created arc, or -1 on failure
 */
int petri_net_add_arc(PetriNet* net, 
                      uint32_t place_id, 
                      uint32_t transition_id,
                      ArcType type, 
                      uint32_t weight);

/**
 * Get a place by ID
 * 
 * @param net The Petri net
 * @param place_id The ID of the place
 * @return Pointer to the place, or NULL if not found
 */
Place* petri_net_get_place(PetriNet* net, uint32_t place_id);

/**
 * Get a place by name
 * 
 * @param net The Petri net
 * @param name The name of the place
 * @return Pointer to the place, or NULL if not found
 */
Place* petri_net_get_place_by_name(PetriNet* net, const char* name);

/**
 * Get a transition by ID
 * 
 * @param net The Petri net
 * @param transition_id The ID of the transition
 * @return Pointer to the transition, or NULL if not found
 */
Transition* petri_net_get_transition(PetriNet* net, uint32_t transition_id);

/**
 * Get a transition by name
 * 
 * @param net The Petri net
 * @param name The name of the transition
 * @return Pointer to the transition, or NULL if not found
 */
Transition* petri_net_get_transition_by_name(PetriNet* net, const char* name);

/**
 * Check if a specific transition is enabled
 * 
 * @param net The Petri net
 * @param transition_id ID of the transition
 * @return true if enabled, false otherwise
 */
bool petri_net_is_transition_enabled(PetriNet* net, uint32_t transition_id);

/**
 * Update the enabled status of all transitions
 * 
 * @param net The Petri net
 */
void petri_net_update_enabled_transitions(PetriNet* net);

/**
 * Fire a specific transition if enabled
 * 
 * @param net The Petri net
 * @param transition_id ID of the transition
 * @return true if the transition fired, false otherwise
 */
bool petri_net_fire_transition(PetriNet* net, uint32_t transition_id);

/**
 * Fire all enabled transitions in the Petri net
 * 
 * @param net The Petri net
 * @return Number of transitions fired
 */
int petri_net_fire_enabled_transitions(PetriNet* net);

/**
 * Get the current marking (token distribution) of the Petri net
 * 
 * @param net The Petri net
 * @param marking Array to store token counts (must be at least net->place_count in size)
 */
void petri_net_get_marking(PetriNet* net, uint32_t* marking);

/**
 * Set the marking (token distribution) of the Petri net
 * 
 * @param net The Petri net
 * @param marking Array of token counts (must be at least net->place_count in size)
 * @return true if successful, false if any place would exceed capacity
 */
bool petri_net_set_marking(PetriNet* net, const uint32_t* marking);

/**
 * Print the current state of the Petri net
 * 
 * @param net The Petri net
 */
void petri_net_print(PetriNet* net);

/**
 * Run a simulation of the Petri net for a specified number of steps
 * 
 * @param net The Petri net
 * @param steps Number of steps to simulate
 * @param print_steps Whether to print the state after each step
 * @return Number of transitions fired during simulation
 */
int petri_net_simulate(PetriNet* net, int steps, bool print_steps);

/**
 * Check if the Petri net is in a deadlock state (no enabled transitions)
 * 
 * @param net The Petri net
 * @return true if deadlocked, false otherwise
 */
bool petri_net_is_deadlocked(PetriNet* net);

#endif /* PETRI_NET_H */