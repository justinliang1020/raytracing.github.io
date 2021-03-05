#ifndef CAMERA_H
#define CAMERA_H
//==============================================================================================
// Originally written in 2016 by Peter Shirley <ptrshrl@gmail.com>
//
// To the extent possible under law, the author(s) have dedicated all copyright and related and
// neighboring rights to this software to the public domain worldwide. This software is
// distributed without any warranty.
//
// You should have received a copy (see file COPYING.txt) of the CC0 Public Domain Dedication
// along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
//==============================================================================================

#include "rtweekend.h"


class camera {
    public:
        camera() : camera(point3(0,0,-1), point3(0,0,0), vec3(0,1,0), 40, 1, 0, 10) {}

        camera(
            point3 lookfrom,
            point3 lookat,
            vec3   vup,
            double vfov, // vertical field-of-view in degrees
            double aspect_ratio,
            double aperture,
            double focus_dist,
            double _time0 = 0,
            double _time1 = 0
        ) {
            this->vfov = vfov;
            this->focus_dist = focus_dist;
            this->vup = vup;
            this->aspect_ratio = aspect_ratio;

            auto theta = degrees_to_radians(vfov);
            auto h = tan(theta/2);
            auto viewport_height = 2.0 * h;
            auto viewport_width = aspect_ratio * viewport_height;

            w = unit_vector(lookfrom - lookat);
            u = unit_vector(cross(vup, w));
            v = cross(w, u);

            origin = lookfrom;
            this->lookat = lookat;
            horizontal = focus_dist * viewport_width * u;
            vertical = focus_dist * viewport_height * v;
            lower_left_corner = origin - horizontal/2 - vertical/2 - focus_dist*w;

            lens_radius = aperture / 2;
            time0 = _time0;
            time1 = _time1;
        }

        ray get_ray(double s, double t) const {
            vec3 rd = lens_radius * random_in_unit_disk();
            vec3 offset = u * rd.x() + v * rd.y();
            return ray(
                origin + offset,
                lower_left_corner + s*horizontal + t*vertical - origin - offset,
                random_double(time0, time1)
            );
        }
        
        point3 get_origin() const {
            return origin;
        }

        void vertical_clamp(vec3& point) {
            point.e[1] = clamp(point.y(), 0.1, 50);
        }

        //TODO: prevent player from moving underground
        /*Move camera where direction is relative to camera perspective 
        */
        void move_cam(vec3 direction, double magnitude) {
            direction = unit_vector(direction);
            vec3 cam_unit = unit_vector(vec3(lookat.x() - origin.x(), lookat.y() - origin.y(), lookat.z() - origin.z()));
            bool pos;
            if (direction == vec3(0, 0, 1)) {   //forwards
                pos = true;
            }
            else if (direction == vec3(0, 0, -1)) { //backwards
                pos = false;
            }
            else if (direction == vec3(1, 0, 0)) {  //right
                cam_unit = vec3(-cam_unit.z(), 0, cam_unit.x()); //rotate cam_unit by right angle about y-axis
                pos = true;
            }
            else if (direction == vec3(-1, 0, 0)) { //left
                cam_unit = vec3(-cam_unit.z(), 0, cam_unit.x()); //rotate cam_unit by right angle about y-axis
                pos = false;
            }
            else if (direction == vec3(0, 1, 0)) {  //up
                cam_unit = vec3(0, 1, 0);
                pos = true;
            }
            else if (direction == vec3(0, -1, 0)) { //down
                cam_unit = vec3(0, 1, 0);
                pos = false;
            }
            if (pos) {
                lookat += cam_unit * magnitude;
                origin += cam_unit * magnitude;
            }
            else {
                lookat -= cam_unit * magnitude;
                origin -= cam_unit * magnitude;
            }
            vertical_clamp(origin);
            if (origin.y() == 0.1 && (direction == vec3(0, 1, 0) || direction == vec3(0, -1, 0))) {
                vertical_clamp(lookat); //clamp vertical movement camera from glitching
            }
        }

        /*Rotate where axis of rotation (unit vector of axis) is relative to camera perspective
        */
        void rotate_cam(vec3 axis, double degrees) {
            axis = unit_vector(axis);
            double radians = degrees_to_radians(degrees);
            double s = std::sin(radians);
            double c = std::cos(radians);
            
            //subtract pivot(origin) from lookat
            double x_prime = lookat.x() - origin.x();
            double y_prime = lookat.y() - origin.y();
            double z_prime = lookat.z() - origin.z();

            //rotate about assuming at (0,0)
            if (axis == vec3(1, 0, 0)) {
                y_prime = y_prime * c - z_prime * s;
                z_prime = y_prime * s + z_prime * c;
            }
            else if (axis == vec3(0, 1, 0)) { 
                x_prime = x_prime * c - z_prime * s;
                z_prime = x_prime * s + z_prime * c;
            }
            else if (axis == vec3(0, 0, 1)) {
                x_prime = y_prime * c - z_prime * s;
                y_prime = y_prime * s + z_prime * c;
            }
            else {
                std::cerr << "invalid camera axis rotation";
                //exception?
            }

            //add back pivot(origin) to lookat
            x_prime += origin.x();
            y_prime += origin.y();
            z_prime += origin.z();

            lookat = vec3(x_prime, y_prime, z_prime);   //memory leak?
        }

        void reset_cam() {
            auto theta = degrees_to_radians(vfov);
            auto h = tan(theta / 2);
            auto viewport_height = 2.0 * h;
            auto viewport_width = aspect_ratio * viewport_height;
                
            w = unit_vector(origin - lookat);
            u = unit_vector(cross(vup, w));
            v = cross(w, u);

            horizontal = focus_dist * viewport_width * u;
            vertical = focus_dist * viewport_height * v;
            lower_left_corner = origin - horizontal / 2 - vertical / 2 - focus_dist * w;
        }

        bool vert_flip() {      //weird camera thing bad fix
            if (lookat.z() - origin.z() < 0)
                return true;
            return false;
        }

    private:
        point3 origin;
        point3 lookat;
        point3 lower_left_corner;
        vec3 horizontal;
        vec3 vertical;
        vec3 u, v, w;
        vec3 vup;
        double lens_radius;
        double time0, time1;  // shutter open/close times
        double vfov;
        double focus_dist;
        double aspect_ratio;
};

#endif
