$fn = 100; /* number of facets per circle */

/*
	_h = height
	_w = width
	_r = radius
	_t = tolerance
	_s = scaling factor (printer specific)
*/

dim_s = 1.04;
kube_s = [dim_s, dim_s, dim_s];

magnet_h = 5;
magnet_t = 0.2;
magnet_r = 13.5 + magnet_t;

flk_h = 23.5;
flk_t = 0.2;
flk_w = 30.5 + 2*flk_t;

wall_w = 2;
outer_w = flk_w + 2*wall_w;

support_r = 5;
support_h = outer_w - wall_w - flk_h;

echo("scaling vector", kube_s);
echo("magnet diameter", magnet_r*2);
echo("flk width", flk_w);
echo("kube width", outer_w);
echo("support height", support_h);

module kube()
{
	difference() {
		cube(outer_w);
		translate([wall_w, wall_w, wall_w + magnet_h]) {
			cube(flk_w, flk_w, outer_w - (wall_w + magnet_h));
		}
		translate([outer_w/2, outer_w/2, wall_w]) {
			cylinder(r=magnet_r, h=magnet_h);
		}
	}

	translate([outer_w/2, outer_w/2, wall_w]) {
		cylinder(r=support_r, h=support_h);
	}
}

scale(kube_s) kube();
