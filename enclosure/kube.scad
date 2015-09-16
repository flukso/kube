/*
	kube.scad - OpenSCAD code generating the Fluksokube enclosure

	Copyright (C) 2014 Bart Van Der Meerssche <bart@flukso.net>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

$fn = 100; /* number of facets per circle */

/*
	_h = height
	_w = width
	_r = radius
	_g = gap
	_t = tolerance
	_s = scaling factor (printer specific)
*/

dim_s = 1;
dim_s = 1.015; /* makerbot */
kube_s = [dim_s, dim_s, dim_s];

magnet_h = 5;
magnet_t = 0.1;
magnet_r = 8 - magnet_t; /* inner diameter now! */

flk_h = 23.5;
flk_t = 0.2;
flk_w = 30.5 + 2*flk_t;

wall_w = 1.5;
outer_w = flk_w + 2*wall_w;

support_h = outer_w - wall_w - flk_h;
support_r = magnet_r;
support_w = 1.5;
support_g = 3;

ring_h = support_h - magnet_h;
ring_t = 0.2;
ring_inner_r = support_r + ring_t;
ring_outer_r = ring_inner_r + 2;

snap_w = 0.4;
snap_t = 0.4;
snap_h = support_h - magnet_h - snap_w - snap_t;
snap_r = support_r + snap_w;

hole_r = 1.5;

echo("*** kube dimensions ***");
echo("scaling vector", "=", kube_s);
echo("magnet diameter", "=", magnet_r*2);
echo("flk width", "=", flk_w);
echo("kube width", "=", outer_w);
echo("support diameter", "=", support_r*2);
echo("support height", "=", support_h);
echo("ring height", "=", ring_h);
echo("ring inner diameter", "=", ring_inner_r*2);
echo("ring outer diameter", "=", ring_outer_r*2);
echo("***********************");

module kube()
{
	difference() {
		cube(outer_w);
		translate([wall_w, wall_w, wall_w]) {
			cube([flk_w, flk_w, outer_w - wall_w]);
		}
		translate([outer_w/2, outer_w/2, 0]) cylinder(r=hole_r, h=wall_w);
	}

	translate([outer_w/2, outer_w/2, wall_w]) {
		rotate(a=[0, 0, 45]) difference() {
			union() {
				cylinder(r=support_r, h=support_h);
				translate([0, 0, support_h - snap_h]) cylinder(r1=snap_r, r2=support_r, h=snap_h);
				translate([0, 0, support_h - snap_h - snap_w]) cylinder(r1=support_r, r2=snap_r, h=snap_w);
			}
			cylinder(r=support_r - support_w, h=support_h);
			translate([-snap_r, -support_g/2, 0]) cube([snap_r*2, support_g, support_h]);
			translate([-support_g/2, -snap_r, 0]) cube([support_g, snap_r*2, support_h]);
		}
	}
}

scale(kube_s) kube();
