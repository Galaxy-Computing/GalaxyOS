#ifndef _KERNEL_KERNEL_H
#define _KERNEL_KERNEL_H

// "GalaxyOS (release name) (release version)-(branch)+(build num.)"
// Build number is incremented with each commit to dev that changes code (that is not the K_VERSION) or is otherwise significant enough to warrant a new version
// If it's not a commit to dev, increment the number after the decimal place. Reset the second number to 0 when the first one is incremented.
#define K_VERSION "GalaxyOS Neptune 0.1.0-dev+13.0" 

extern int bootfinished;

#endif