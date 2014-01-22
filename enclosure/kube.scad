module kube()
{
	difference() {
		cube(35, 35, 35);
		translate([2, 2, 7]) {
			cube(31, 31, 28);
		}
		translate([17.5, 17.5, 2]) {
			cylinder(r=13.7, h=5);	/* r(magnet) = 13.5 */
		}
	}

	translate([17.5, 17.5, 2]) {
		cylinder(r=5, h=9.5);
	}
}

kube();