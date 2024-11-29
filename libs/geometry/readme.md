# geometry
Implmentuje geometricke algoritmy štruktúry a pomocné funkcie.

> TODO: What we have there? How to use stafff we have there?

- `utility.hpp`
- `box2.hpp`
- `box3.hpp`
- `glmprint.hpp`
- `boost_geometry_adapt.hpp`

> TODO: I rememer that I've renamed this to `glm_adapt.hpp`, but

## geometry:inštalácia

Nainštaluj nasledujúce balíčky

* libglm-dev (0.9.9, ubuntu 18.04)

príkazom `$ sudo apt install libglm-dev`


## geometry:štruktúra

**box2.hpp** : implementácia modelu obdlžníka

```c++
box2 b = make_inverse<box2>();
expand(b, vec2{3,3});
expand(b, vec2{5,5});
box2 c = make<box2>(3,3, 5,5);
// b == c
```

**box3.hpp** : implementácia modelu kvádra

```c++
box3 b = make<box3>(0,0,0, 1,1,1);
```

**utility.hpp** : funkcie pracujúce s rotáciou

```c++
quat r = angleAxis(radians(60.0f), vec3{0,0,1});
cout << "right=" << right(r) << "\n"
	<< "up=" << up(r) << "\n"
	<< "forward=" << forward(r) << "\n";
```

**glmprint.hpp** : formátovaný výstup

```c++
mat4 M{1};
cout << with_label("M", M);
```

**boost_geometry_adapt.hpp** : algoritmy s knižnice `boost::geometry`


## geometry:ukážky

```c++
// expand algorithm test
#include <iostream>
#include <boost/geometry/geometry.hpp>
#include "box2.hpp"

using geom::box2;
using geom::make;
using geom::make_inverse;
using geom::expand;

int main(int argc, char * argv[])
{
	box2 b = make_inverse<box2>();
	expand(b, glm::vec2{0,0});
	expand(b, glm::vec2{1,2});
	expand(b, glm::vec2{5,4});
	expand(b, make<box2>(3,3, 5,5));

	std::cout << boost::geometry::dsv(b) << std::endl;

	return 0;
}
```
