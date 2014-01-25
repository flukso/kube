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

/*$fn = 100; /* number of facets per circle */

/*
	_h = height
	_w = width
	_r = radius
	_t = tolerance
	_s = scaling factor (printer specific)
*/

dim_s = 1;
dim_s = 1.015; /* makerbot */
kube_s = [dim_s, dim_s, dim_s];

magnet_h = 5;
magnet_t = 0.2;
magnet_r = 8 - magnet_t; /* inner diameter now! */

flk_h = 23.5;
flk_t = 0.2;
flk_w = 30.5 + 2*flk_t;

wall_w = 2;
outer_w = flk_w + 2*wall_w;

support_h = outer_w - wall_w - flk_h;
support_r = magnet_r;

ring_h = support_h - magnet_h;
ring_t = 0.1;
ring_inner_r = support_r + ring_t;
ring_outer_r = ring_inner_r + 2;

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
	}

	translate([outer_w/2, outer_w/2, wall_w]) {
		cylinder(r=support_r, h=support_h);
	}
}

module ring()
{
	difference() {
		cylinder(r=ring_outer_r, h=ring_h);
		cylinder(r=ring_inner_r, h=ring_h);
	}
}

scale(kube_s) {
	kube();
	translate([1.5*outer_w, outer_w/2, 0]) ring();
}
