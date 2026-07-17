#ifndef MM_Animal_H__
#define MM_Animal_H__
#include "dog_cat.h"


struct MM_AnimalInterface {
  typeof(dog_eat)* _eat;
  typeof(dog_speak)* _speak;
  typeof(dog_sleep)* _sleep;
};

extern struct MM_AnimalInterface mm_Animal_iface;

extern const struct MM_AnimalInterface mm_Dog_vt;
extern const struct MM_AnimalInterface mm_Cat_vt;

#define eat mm_Animal_iface.eat
#define speak mm_Animal_iface.speak
#define sleep mm_Animal_iface.sleep


#ifdef MM_Animal_IMPLEMENTATION

const struct MM_AnimalInterface mm_Dog_vt = {
  dog_eat,
  dog_speak,
  dog_sleep
};

const struct MM_AnimalInterface mm_Cat_vt = {
  cat_eat,
  cat_speak,
  cat_sleep
};

struct MM_AnimalInterface mm_Animal_iface = {
  dog_eat,
  dog_speak,
  dog_sleep
};


#endif
#endif

