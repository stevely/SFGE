/*
 * renderer.c
 * By Steven Smith
 */

#include <stdio.h>
#include "renderer.h"

GLFWwindow sgfeCreateWindow() {
    GLFWwindow window;
    /* Hard-coded values for now */
    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_OPENGL_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    window = glfwCreateWindow(800, 800, GLFW_WINDOWED, "SGFE Window", NULL);
    if( window == NULL ) {
        printf("Failed to open window!\n");
        printf("Error: %s\n", glfwErrorString(glfwGetError()));
        return NULL;
    }
    glfwMakeContextCurrent(window);
    return window;
}

int renderLoop( GLFWwindow window, sstProgram *program ) {
    GLfloat proj[16], base[16], model[16];
    sgfeRenderBuffers *buffer;
    sgfeDrawable *ds;
    int i;
    buffer = sgfeGetConsumerBuffer();
    sstPerspectiveMatrix_(60.0f, 1.0f, 5.0f, 500.0f, proj);
    sstActivateProgram(program);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glViewport(0, 0, 800, 800);
    sstSetUniformData(program, "projectionMatrix", proj);
    /* Gotta sit out the first frame since the first dataset hasn't been made */
    buffer = sgfeSwapBuffers(buffer);
    while( buffer ) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        /* First entry in the buffer is the camera */
        ds = buffer->drawables;
        sstRotateMatrixX_(ds->rot[1], base);
        sstRotateMatrixY_(ds->rot[0], model);
        sstMatMult4_(base, model, base);
        sstTranslateMatrixInto(-ds->pos[0], -ds->pos[1], -ds->pos[2], base);
        sstSetUniformData(program, "lightPos1", ds->pos);
        sstSetUniformData(program, "cameraPos", ds->pos);
        sstSetUniformData(program, "cameraMatrix", base);
        /* Draw everything */
        ds++;
        for( i = 1; i < buffer->draw_count; i++, ds++ ) {
            sstTranslateMatrix_(ds->pos[0], ds->pos[1], ds->pos[2], model);
            sstSetUniformData(program, "modelTranslate", model);
            sstRotateMatrixX_(ds->rot[0], base);
            sstRotateMatrixY_(ds->rot[1], model);
            sstMatMult4_(base, model, model);
            sstSetUniformData(program, "modelRotate", model);
            sstDrawSet(ds->set);
        }
        glFlush();
        glfwSwapBuffers(window);
        /* Event polling has to be done here due to rendering being done on the
         * main thread. This is necessary because GLFW is not thread safe, and
         * while checking key/mouse state in a separate thread will work,
         * actually polling will not.
         */
        glfwPollEvents();
        buffer = sgfeSwapBuffers(buffer);
    }
    return 0;
}
