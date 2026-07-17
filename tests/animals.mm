# this is animals.mm file, this is a comment


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

# we set it to Dog as default
# here, so if we call the function speak(), it will call
# dog_speak() instead....
$export Animal as Dog


# This is a side-project im working on
# im extending C, so it can have "interfaces"
# Animal can eat, sleep, and speak
# but each animal does it differently
# Dog -> BHAUUUUUUU WOOOF WOOF, arav
# Cat -> MEOOWWWWWWWWW

