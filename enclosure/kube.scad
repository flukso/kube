/*
	_h = height
	_w = width
	_r = radius
	_t = tolerance
*/

magnet_h = 5;
magnet_r = 13.5;
magnet_t = 0.2;

flk_h = 23.5;
flk_w = 30.5;
flk_t = 0.5;

wall_w = 2;

support_r = 5;
support_h = outer_w - wall_w - flk_h;

inner_w = flk_w + flk_t;
outer_w = inner_w + 2*wall_w;

module kube()
{
	difference() {
		cube(outer_w);
		translate([wall_w, wall_w, wall_w + magnet_h]) {
			cube(inner_w, inner_w, outer_w - (wall_w + magnet_h));
		}
		translate([outer_w/2, outer_w/2, wall_w]) {
			cylinder(r=magnet_r + magnet_t, h=magnet_h);
		}
	}

	translate([outer_w/2, outer_w/2, wall_w]) {
		cylinder(r=support_r, h=support_h);
	}
}

kube();