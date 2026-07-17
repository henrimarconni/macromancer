#ifndef ANIMALS_H
#define ANIMALS_H

#include "dog_cat.h"

typedef struct {
  typeof(&dog_eat) _eat;
  typeof(&dog_sleep) _sleep;
  typeof(&dog_speak) _speak;
} MacroMancerAnimalVTable;


extern MacroMancerAnimalVTable macro_mancer_animal_vtable;

#define eat macro_mancer_animal_vtable._eat
#define sleep macro_mancer_animal_vtable._sleep
#define speak macro_mancer_animal_vtable._speak



#endif
