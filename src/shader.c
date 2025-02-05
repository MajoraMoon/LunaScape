#include <shader.h>

// intern function to read glsl shader files
static char *readFile(const char *filePath) {
  // open file
  FILE *file = fopen(filePath, "r");
  if (!file) {
    printf("ERROR: Unable to open file %s\n", filePath);
    return NULL;
  }

  // sets pointer in the file to the end of the file.s
  fseek(file, 0, SEEK_END);

  // gives back the size of the file based on the pointer position.
  // here it is placed at the end of the file, so it gets the whole size.
  long legth = ftell(file);

  // sets pointer to the start again.
  rewind(file);

  char *buffer = (char *)malloc(legth + 1);

  if (!buffer) {
    printf("ERROR: Memory allocation failed, trying to reading a file...\n");
    fclose(file);
    return NULL;
  }

  fread(buffer, 1, legth, file);

  // adding the null character..
  buffer[legth] = '\0';

  fclose(file);

  return buffer;
}

// checks for shader compilation errors
static void checkShaderCompileErrors(unsigned int shader, const char *type) {
  int success;
  char infoLog[1024];

  // maybe a little bit confusing here, anyway. that means:
  // throwing compiling error when the fragment or vertex shader fails,
  // otherwise it's a program failure :)
  if (type != "PROGRAM") {
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
      glGetShaderInfoLog(shader, 1024, NULL, infoLog);
      printf("ERROR::SHADER::%s::COMPILATION_FAILED\n%s\n", type, infoLog);
    } else {
      glGetProgramiv(shader, GL_LINK_STATUS, &success);
      if (!success) {
        glGetProgramInfoLog(shader, 1024, NULL, infoLog);
        printf("ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
      }
    }
  }
}

Shader createShader(const char *vertexPath, const char *fragmentPath) {
  Shader shader;
  shader.ID = 0;

  char *vertexData = readFile(vertexPath);
  char *fragmentData = readFile(fragmentPath);

  if (!vertexData || !fragmentData) {
    printf("ERROR: Shader source code could not be read\n");
    free(vertexData);
    free(fragmentData);
    return shader;
  }

  // compile vertexShader
  unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, (const char *const *)&vertexData, NULL);
  glCompileShader(vertexShader);
  checkShaderCompileErrors(vertexShader, "VERTEX");

  // compile fragment shader
  unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, (const char *const *)&fragmentData, NULL);
  glCompileShader(fragmentShader);
  checkShaderCompileErrors(fragmentShader, "FRAGMENT");

  // links (compiles) shader together
  shader.ID = glCreateProgram();
  glAttachShader(shader.ID, vertexShader);
  glAttachShader(shader.ID, fragmentShader);
  glLinkProgram(shader.ID);
  checkShaderCompileErrors(shader.ID, "PROGRAM");

  // shaders are compiled and linked, if I'm right, I can delete the source..
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  // I guess, I can free the data I've read from the files too.
  free(vertexData);
  free(fragmentData);

  return shader;
}

void useShader(const Shader *shader) { glUseProgram(shader->ID); }

void deleteShader(Shader *shader) {
  glDeleteProgram(shader->ID);
  shader->ID = 0;
}