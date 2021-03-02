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

#include "camera.h"
#include "color.h"
#include "hittable_list.h"
#include "material.h"
#include "sphere.h"

#include <SDL.h>
#include <emscripten.h>

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdint.h>



// Image

const auto aspect_ratio = 16.0 / 9.0;
const int image_width = 250;
const int image_height = static_cast<int>(image_width / aspect_ratio);
const int samples_per_pixel = 1; // min = 1
const int max_depth = 2; //min = 2

// Camera

point3 lookfrom(0, 2, -10);
point3 lookat(0, 0, 0);
vec3 vup(0, 1, 0);
auto dist_to_focus = 10.0;
auto aperture = 0.1;

camera cam(lookfrom, lookat, vup, 20, aspect_ratio, aperture, dist_to_focus);

// SDL

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Surface* surface;
SDL_Event e;



color ray_color(const ray& r, const hittable& world, int depth) {
    hit_record rec;

    // If we've exceeded the ray bounce limit, no more light is gathered.
    if (depth <= 0)
        return color(0,0,0);

    if (world.hit(r, 0.001, infinity, rec)) {
        ray scattered;
        color attenuation;
        if (rec.mat_ptr->scatter(r, rec, attenuation, scattered))
            return attenuation * ray_color(scattered, world, depth-1);
        return color(0,0,0);
    }

    vec3 unit_direction = unit_vector(r.direction());
    auto t = 0.5*(unit_direction.y() + 1.0);
    return (1.0-t)*color(1.0, 1.0, 1.0) + t*color(0.5, 0.7, 1.0);
}


hittable_list random_scene() {
    hittable_list world;

    auto ground_material = make_shared<lambertian>(color(0.5, 0.5, 0.5));
    world.add(make_shared<sphere>(point3(0,-1000,0), 1000, ground_material));

    for (int a = -11; a < 11; a++) {
        for (int b = -11; b < 11; b++) {
            auto choose_mat = random_double();
            point3 center(a + 0.9*random_double(), 0.2, b + 0.9*random_double());

            if ((center - point3(4, 0.2, 0)).length() > 0.9) {
                shared_ptr<material> sphere_material;

                if (choose_mat < 0.8) {
                    // diffuse
                    auto albedo = color::random() * color::random();
                    sphere_material = make_shared<lambertian>(albedo);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                } else if (choose_mat < 0.95) {
                    // metal
                    auto albedo = color::random(0.5, 1);
                    auto fuzz = random_double(0, 0.5);
                    sphere_material = make_shared<metal>(albedo, fuzz);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                } else {
                    // glass
                    sphere_material = make_shared<dielectric>(1.5);
                    world.add(make_shared<sphere>(center, 0.2, sphere_material));
                }
            }
        }
    }

    auto material1 = make_shared<dielectric>(1.5);
    world.add(make_shared<sphere>(point3(0, 1, 0), 1.0, material1));

    auto material2 = make_shared<lambertian>(color(0.4, 0.2, 0.1));
    world.add(make_shared<sphere>(point3(-4, 1, 0), 1.0, material2));

    auto material3 = make_shared<metal>(color(0.7, 0.6, 0.5), 0.0);
    world.add(make_shared<sphere>(point3(4, 1, 0), 1.0, material3));

    return world;
}

hittable_list test_scene() {
    hittable_list world;

    auto ground_material = make_shared<lambertian>(color(0.5, 0.5, 0.5));
    world.add(make_shared<sphere>(point3(0, -1000, 0), 1000, ground_material));

    point3 center(5, 0.4, 2);
    point3 center2(3, 0.2, 1);
    auto albedo = color::random(0.6, 1);
    auto albedo2 = color::random(0.3, 0.6);
    auto fuzz = random_double(0, 0.5);

    shared_ptr<material> sphere_material = make_shared<metal>(albedo, fuzz);
    shared_ptr<material> sphere_material2 = make_shared<metal>(albedo2, fuzz);
    world.add(make_shared<sphere>(center, 0.4, sphere_material));
    world.add(make_shared<sphere>(center2, 0.2, sphere_material2));

    return world;
}

// World

auto world = test_scene();

void add_sphere(point3 center) {
    auto albedo = color::random(0.6, 1);
    auto fuzz = random_double(0, 0.5);
    shared_ptr<material> sphere_material = make_shared<metal>(albedo, fuzz);

    world.add(make_shared<sphere>(center, 0.3, sphere_material));
}

void add_sphere_random() {
    point3 center = point3(random_double(-3, 3), random_double(0, 3), random_double(-3, 3));
    add_sphere(center);
}

void ppm_render(int ppm_image_width, int ppm_max_depth, int ppm_samples_per_pixel) {
    int ppm_image_height = static_cast<int>(ppm_image_width / aspect_ratio);

    //fix later
    std::ofstream out("out.ppm");
    std::streambuf* coutbuf = std::cout.rdbuf(); //save old buf
    std::cout.rdbuf(out.rdbuf()); //redirect std::cout to out.ppm

    std::cout << "P3\n" << ppm_image_width << ' ' << ppm_image_height << "\n255\n";

    for (int j = ppm_image_height - 1; j >= 0; --j) {
        std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush;
        for (int i = 0; i < ppm_image_width; ++i) {
            color pixel_color(0, 0, 0);
            for (int s = 0; s < ppm_samples_per_pixel; ++s) {
                auto u = (i + random_double()) / (ppm_image_width - 1);
                auto v = (j + random_double()) / (ppm_image_height - 1);
                ray r = cam.get_ray(u, v);
                pixel_color += ray_color(r, world, ppm_max_depth);
            }
            write_color(std::cout, pixel_color, ppm_samples_per_pixel);
        }
    }
    std::cerr << "\nDone.\n";
}

void player_input() {
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
            //translations
            case SDLK_w:
                cam.move_cam(vec3(0, 0, 1), 1.0);  //forward
                break;
            case SDLK_s:
                cam.move_cam(vec3(0, 0, -1), 1.0); //backwards
                break;
            case SDLK_a:
                cam.move_cam(vec3(-1, 0, 0), 0.5); //left
                break;
            case SDLK_d:
                cam.move_cam(vec3(1, 0, 0), 0.5);  //right
                break;
            case SDLK_SPACE:
                cam.move_cam(vec3(0, 1, 0), 0.5);   //up
                break;
            case SDLK_LCTRL:
                cam.move_cam(vec3(0, -1, 0), 0.5);  //down
                break;
            //rotations
            case SDLK_UP:   //up
                if (cam.vert_flip()) {  // BAD FIX, must redo entire camera system laters
                    cam.rotate_cam(vec3(1, 0, 0), 5);
                }
                else {
                    cam.rotate_cam(vec3(1, 0, 0), -5);
                }
                break;
            case SDLK_DOWN: //down
                if (cam.vert_flip()) {
                    cam.rotate_cam(vec3(1, 0, 0), -5);
                }
                else {
                    cam.rotate_cam(vec3(1, 0, 0), 5);
                }
                break;
            case SDLK_LEFT:
                cam.rotate_cam(vec3(0, 1, 0), -5);  //left
                break;
            case SDLK_RIGHT:
                cam.rotate_cam(vec3(0, 1, 0), 5);   //right
                break;
            //screenshot
            case SDLK_p:
                ppm_render(500, 30, 30);
                break;
            //add sphere
            case SDLK_r:
                add_sphere_random();
                break;
            }

            cam.reset_cam();
        }
    }
}

void drawSurface() {
    if (SDL_MUSTLOCK(surface)) SDL_LockSurface(surface);

    Uint8* pixels = (Uint8*)surface->pixels;

    player_input();

    for (int j = image_height - 1; j >= 0; --j) {
        for (int i = 0; i < image_width; ++i) {
            color pixel_color(0, 0, 0);
            for (int s = 0; s < samples_per_pixel; ++s) {
                auto u = (i + random_double()) / (image_width - 1);
                auto v = (j + random_double()) / (image_height - 1);
                ray r = cam.get_ray(u, v);
                pixel_color += ray_color(r, world, max_depth);
            }
            auto r = pixel_color.x();
            auto g = pixel_color.y();
            auto b = pixel_color.z();

            // Replace NaN components with zero. See explanation in Ray Tracing: The Rest of Your Life.
            if (r != r) r = 0.0;
            if (g != g) g = 0.0;
            if (b != b) b = 0.0;

            // Divide the color by the number of samples and gamma-correct for gamma=2.0.
            auto scale = 1.0 / samples_per_pixel;
            r = sqrt(scale * r);
            g = sqrt(scale * g);
            b = sqrt(scale * b);

            int pixel_index = (i + (image_height - 1 - j) * image_width) * 4;
            pixels[pixel_index] = 256 * clamp(b, 0.0, 0.999); //blue
            pixels[pixel_index + 1] = 256 * clamp(g, 0.0, 0.999); //green
            pixels[pixel_index + 2] = 256 * clamp(r, 0.0, 0.999); //red
            pixels[pixel_index + 3] = 100; //alpha
        }
    }
    if (SDL_MUSTLOCK(surface)) SDL_UnlockSurface(surface);

    SDL_Texture* screenTexture = SDL_CreateTextureFromSurface(renderer, surface);

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, screenTexture, NULL, NULL);
    SDL_RenderPresent(renderer);

    SDL_DestroyTexture(screenTexture);
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(image_width, image_height, 0, &window, &renderer);
    surface = SDL_CreateRGBSurface(0, image_width, image_height, 32, 0, 0, 0, 0);

    emscripten_set_main_loop(drawSurface, 0, 1);
}