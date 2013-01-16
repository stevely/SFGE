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
    sgfeEntity *buffer, *b;
    int buf_size, i;
    glfwMakeContextCurrent(window);
    buffer = sgfeGetBuffer(0); /* Renderer is a consumer */
    sstPerspectiveMatrix_(60.0f, 1.0f, 5.0f, 500.0f, proj);
    sstActivateProgram(program);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glViewport(0, 0, 800, 800);
    sstSetUniformData(program, "projectionMatrix", proj);
    /* Gotta sit out the first frame since the first dataset hasn't been made */
    buf_size = 0;
    buffer = sgfeSwapBuffers(buffer, &buf_size);
    while( buffer ) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        /* First entry in the buffer is the camera */
        b = buffer;
        sstRotateMatrixX_(b->rot[1], base);
        sstRotateMatrixY_(b->rot[0], model);
        sstMatMult4_(base, model, base);
        sstTranslateMatrixInto(-b->pos[0], -b->pos[1], -b->pos[2], base);
        sstSetUniformData(program, "lightPos1", b->pos);
        sstSetUniformData(program, "cameraPos", b->pos);
        sstSetUniformData(program, "cameraMatrix", base);
        /* Draw everything */
        b++;
        for( i = 1; i < buf_size; i++, b++ ) {
            sstTranslateMatrix_(b->pos[0], b->pos[1], b->pos[2], model);
            sstSetUniformData(program, "modelTranslate", model);
            /* TODO: Rotate */
            sstRotateMatrixX_(b->rot[0], base);
            sstRotateMatrixY_(b->rot[1], model);
            sstMatMult4_(base, model, model);
            sstSetUniformData(program, "modelRotate", model);
            sstDrawSet(b->set);
        }
        glFlush();
        glfwSwapBuffers(window);
        buffer = sgfeSwapBuffers(buffer, &buf_size);
    }
    return 0;
}
