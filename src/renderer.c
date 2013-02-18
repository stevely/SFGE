/*
 * renderer.c
 * By Steven Smith
 */

#include <stdio.h>
#include "renderer.h"

GLFWwindow sfgeCreateWindow() {
    GLFWwindow window;
    /* Hard-coded values for now */
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_OPENGL_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    window = glfwCreateWindow(800, 800, GLFW_WINDOWED, "sfge Window", NULL);
    if( window == NULL ) {
        printf("Failed to open window!\n");
        printf("Error: %s\n", glfwErrorString(glfwGetError()));
        return NULL;
    }
    glfwMakeContextCurrent(window);
    return window;
}

static void render( GLfloat *proj, sfgeRenderBuffers *buffer,
sstProgram *program ) {
    GLfloat base[16], model[16];
    sfgeDrawable *ds;
    int i;
    sstSetUniformData(program, "projectionMatrix", proj);
    /* First entry in the buffer is the camera */
    ds = buffer->drawables;
    sstRotateMatrixX_(ds->rot[1], base);
    sstRotateMatrixY_(ds->rot[0], model);
    sstMatMult4_(base, model, base);
    sstTranslateMatrixInto(-ds->pos[0], -ds->pos[1], -ds->pos[2], base);
    sstSetUniformData(program, "cameraPos", ds->pos);
    sstSetUniformData(program, "cameraMatrix", base);
    /* Draw everything */
    ds++;
    for( i = 1; i < buffer->draw_count; i++ ) {
        sstTranslateMatrix_(ds->pos[0], ds->pos[1], ds->pos[2], model);
        sstSetUniformData(program, "modelTranslate", model);
        sstRotateMatrixX_(ds->rot[0], base);
        sstRotateMatrixY_(ds->rot[1], model);
        sstMatMult4_(base, model, model);
        sstSetUniformData(program, "modelRotate", model);
        sstDrawSet(ds->set);
        ds++;
    }
}

int renderLoop( GLFWwindow window, sstProgram *naked, sstProgram *point ) {
    GLfloat proj[16];
    sfgeRenderBuffers *buffer;
    sfgeLight *light;
    int i;
    sstProgram *program;
    program = NULL;
    buffer = sfgeGetConsumerBuffer();
    sstPerspectiveMatrix_(60.0f, 1.0f, 5.0f, 500.0f, proj);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glViewport(0, 0, 800, 800);
    glBlendFunc(GL_ONE, GL_ONE);
    /* Gotta sit out the first frame since the first dataset hasn't been made */
    buffer = sfgeSwapBuffers(buffer);
    while( buffer ) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        /* We use multiple render passes to draw the scene. To speed things up
         * we first run a "naked" pass that does no work beyond geometry
         * transformations to establish the depth buffer and eliminate over-
         * draw. */
        /* TODO: Set up multiple shader programs */
        if( naked ) {
            glDepthFunc(GL_LESS);
            render(proj, buffer, naked);
            glDepthFunc(GL_EQUAL);
        }
        /* Time to render the scene for every light source */
        light = buffer->lights;
        for( i = 0; i < buffer->light_count; i++ ) {
            switch( light->type ) {
            case pointL:
                if( program != point ) {
                    sstActivateProgram(point);
                }
                program = point;
                sstSetUniformData(point, "lightPos1", light->light.point.pos);
                sstSetUniformData(point, "lightColor1", light->light.point.color);
                sstSetUniformData(point, "lightStart1", &light->light.point.r_start);
                sstSetUniformData(point, "lightEnd1", &light->light.point.r_end);
                break;
            case directionalL:
            case spotlightL:
            case noneL:
            default:
                program = point;
                break;
            }
            render(proj, buffer, program);
            light++;
        }
        glFlush();
        glfwSwapBuffers(window);
        /* Event polling has to be done here due to rendering being done on the
         * main thread. This is necessary because GLFW is not thread safe, and
         * while checking key/mouse state in a separate thread will work,
         * actually polling will not.
         */
        glfwPollEvents();
        buffer = sfgeSwapBuffers(buffer);
    }
    return 0;
}
