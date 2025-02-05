#ifndef SHADER_H
#define SHADER_H
// clang-format off
#include <glad/glad.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
// clang-format on
typedef struct Shader
{
  unsigned int ID; // OpenGL shader programm ID
} Shader;

// Creates a Shader Program that takes both vertex and fragmaent files as an
// input
Shader createShader(const char *vertexPath, const char *fragmentPath);

// activates the shader programm.
// Use this function while rendering. So in the main-while loop.
void useShader(const Shader *shader);

// deletes the shader Program, not the shader read from the file. Be careful!
// use this when the main-loop is terminated. So everything will end properly.
void deleteShader(Shader *shader);

#endif