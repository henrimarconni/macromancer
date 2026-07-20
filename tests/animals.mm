$interface Animal as Dynamic {
	eat
	speak
	sleep
}

$impl Dog as Animal {
	$header = "dog_cat.h"
	speak = dog_speak
	eat = dog_eat
	sleep = dog_sleep
}

$impl Cat as Animal {
	$header = "dog_cat.h"
	eat = cat_eat
	speak = cat_speak
	sleep = cat_sleep
}

$impl Dawg as Animal {
	$header = "dog_cat.h"
	eat = cat_eat
	speak = cat_speak
	sleep = cat_sleep
}

$impl Dawggy as Animal {
	$header = "dog_cat.h"
	eat = cat_eat
	speak = cat_speak
	sleep = cat_sleep
}

$impl Dawg as Animal {
	$poof = woof
}

$export Animal as Dog
