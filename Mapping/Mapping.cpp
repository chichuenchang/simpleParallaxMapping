#include <windows.h>

#include <GL/glew.h>

#include <GL/freeglut.h>

#include <GL/gl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

#include "InitShader.h"
#include "LoadMeshTangents.h"
#include "LoadTexture.h"
#include "imgui_impl_glut.h"
#include "VideoMux.h"
#include "DebugCallback.h"
#include "ShaderLocs.h"

MeshData mesh_data[4];
GLuint shader_program[2] = {-1};
GLuint diffuse_map_id[4] = {-1}; 
GLuint normal_map_id[4] = {-1};
GLuint bump_map_id[4] = {-1};
GLuint env_map_id = -1;

//names of the shader files to load
static const std::string vertex_shader[] = {"mapping_w_vs.glsl", "mapping_tbn_vs.glsl"};
static const std::string fragment_shader[] = {"mapping_w_fs.glsl", "mapping_tbn_fs.glsl"};

//names of the mesh and texture files to load
static const std::string mesh_name[] = {"cube.obj","plane.obj","sphere.obj","torus.obj"};
//Texture maps from filter forge: https://www.filterforge.com/filters/
static const std::string diffuse_map_name[] = {"235-diffuse.jpg", "1792-diffuse.jpg", "1857-diffuse.jpg", "243-diffuse.jpg"};
static const std::string normal_map_name[] = {"235-normal.jpg", "1792-normal.jpg", "1857-normal.jpg", "243-normal.jpg"};
static const std::string bump_map_name[] = {"235-bump.jpg", "1792-bump.jpg", "1857-bump.jpg", "243-bump.jpg"};
static const std::string env_map_name = "cubemap";

//indices of current objects in above arrays
int current_mesh = 0;
int current_map = 0;
int current_shader = 0;

float time_sec = 0.0f;
float angle = 0.0f;
float cam_z = 2.0f;
bool recording = false;

//Draw the user interface using ImGui
void draw_gui()
{
   ImGui_ImplGlut_NewFrame();

   const int filename_len = 256;
   static char video_filename[filename_len] = "capture.mp4";

   ImGui::InputText("Video filename", video_filename, filename_len);
   ImGui::SameLine();
   if (recording == false)
   {
      if (ImGui::Button("Start Recording"))
      {
         const int w = glutGet(GLUT_WINDOW_WIDTH);
         const int h = glutGet(GLUT_WINDOW_HEIGHT);
         recording = true;
         start_encoding(video_filename, w, h); //Uses ffmpeg
      }
      
   }
   else
   {
      if (ImGui::Button("Stop Recording"))
      {
         recording = false;
         finish_encoding(); //Uses ffmpeg
      }
   }

   //camera controls
   ImGui::SliderFloat("View angle", &angle, -3.141592f, +3.141592f);
   ImGui::SliderFloat("Cam z", &cam_z, -2.0f, +2.0f);
   
   static float alpha = 0.0f;
   if(ImGui::SliderFloat("Alpha", &alpha, 0.0f, 1.0f))
   {
      glUniform1f(UniformLoc::Slider, alpha); 
   }

   ImGui::Columns(4);
   ImGui::Image((void*)diffuse_map_id[current_map], ImVec2(128,128));
   ImGui::Text(diffuse_map_name[current_map].c_str());
   ImGui::NextColumn();
   ImGui::Image((void*)normal_map_id[current_map], ImVec2(128,128));
   ImGui::Text(normal_map_name[current_map].c_str());
   ImGui::NextColumn();
   ImGui::Image((void*)bump_map_id[current_map], ImVec2(128,128));
   ImGui::Text(bump_map_name[current_map].c_str());
   ImGui::NextColumn();

   ImGui::TextColored(ImVec4(1.0, 1.0, 0.0, 1.0), "Texture");
   for(int i=0; i<4; i++)
   {
      ImGui::RadioButton(diffuse_map_name[i].c_str(), &current_map, i);
   }
   
   ImGui::Columns(3);
   ImGui::Separator();
   ImGui::TextColored(ImVec4(1.0, 1.0, 0.0, 1.0), "Mode");
   static int mode = 0;
   ImGui::RadioButton("Show Normal", &mode, 0);
   ImGui::RadioButton("Show Tangent", &mode, 1); 
   ImGui::RadioButton("Show Bitangent", &mode, 2); 
   ImGui::RadioButton("Show Env map", &mode, 3);
   ImGui::RadioButton("Show Env map bump map", &mode, 4); 
   ImGui::RadioButton("Show Lighting", &mode, 5); 
   glUniform1i(UniformLoc::Mode, mode);
   ImGui::NextColumn();
   ImGui::TextColored(ImVec4(1.0, 1.0, 0.0, 1.0), "Mesh");
   for(int i=0; i<4; i++)
   {
      ImGui::RadioButton(mesh_name[i].c_str(), &current_mesh, i);
   }
   ImGui::NextColumn();
   ImGui::TextColored(ImVec4(1.0, 1.0, 0.0, 1.0), "Shader");
   for(int i=0; i<2; i++)
   {
      ImGui::RadioButton(vertex_shader[i].c_str(), &current_shader, i);
   }
   ImGui::Columns(1);

   //ImGui::ShowTestWindow();
   ImGui::Render();
 }

// glut display callback function.
// This function gets called every time the scene gets redisplayed 
void display()
{
   //clear the screen
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glUseProgram(shader_program[current_shader]);

   const int w = glutGet(GLUT_WINDOW_WIDTH);
   const int h = glutGet(GLUT_WINDOW_HEIGHT);
   const float aspect_ratio = float(w) / float(h);

   glm::mat4 M = glm::scale(glm::vec3(mesh_data[current_mesh].mScaleFactor));
   glm::mat4 V = glm::lookAt(glm::vec3(0.1f, 0.5f, cam_z), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.1f))*glm::rotate(angle, glm::vec3(0.0f, 1.0f, 0.0f));
   glm::mat4 P = glm::perspective(3.141592f / 4.0f, aspect_ratio, 0.1f, 100.0f);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, diffuse_map_id[current_map]);
   glUniform1i(UniformLoc::DiffuseTex, 0); // we bound our texture to texture unit 0

   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, normal_map_id[current_map]);
   glUniform1i(UniformLoc::NormalTex, 1); 

   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, bump_map_id[current_map]);
   glUniform1i(UniformLoc::BumpTex, 2); 

   glActiveTexture(GL_TEXTURE3);
   glBindTexture(GL_TEXTURE_CUBE_MAP, env_map_id);
   glUniform1i(UniformLoc::EnvTex, 3); 


   glUniformMatrix4fv(UniformLoc::P, 1, false, glm::value_ptr(P));
   glUniformMatrix4fv(UniformLoc::V, 1, false, glm::value_ptr(V));
   glUniformMatrix4fv(UniformLoc::M, 1, false, glm::value_ptr(M));
   {
      glm::mat4 PVM = P*V*M;
      glUniformMatrix4fv(UniformLoc::PVM, 1, false, glm::value_ptr(PVM));
   }

   glBindVertexArray(mesh_data[current_mesh].mVao);
   mesh_data[current_mesh].DrawMesh();
         
   draw_gui();

   if (recording == true)
   {
      glFinish();

      glReadBuffer(GL_BACK);
      read_frame_to_encode(&rgb, &pixels, w, h);
      encode_frame(rgb);
   }

   glEnable(GL_CULL_FACE);
   glCullFace(GL_BACK);

   glutSwapBuffers();
}

// glut idle callback.
//This function gets called between frames
void idle()
{
	glutPostRedisplay();

   const int time_ms = glutGet(GLUT_ELAPSED_TIME);
   time_sec = 0.001f*time_ms;
}

bool reload_shader(int i)
{
   bool success = true;
   GLuint new_shader = InitShader(vertex_shader[i].c_str(), fragment_shader[i].c_str());

   if(new_shader == -1) // loading failed
   {
      success = false;
   }
   else
   {
      if(shader_program[i] != -1)
      {
         glDeleteProgram(shader_program[i]);
      }
      shader_program[i] = new_shader;
      success = true;
   }
   return success;
}

void reload_shaders()
{
   bool all_loaded = true;
   for(int i=0; i<2; i++)
   {
      bool success = reload_shader(i);
      all_loaded = all_loaded && success;
   }

   if(all_loaded)
   {
      glClearColor(0.35f, 0.35f, 0.35f, 0.0f);
   }
   else
   {
      glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
   }
}

// Display info about the OpenGL implementation provided by the graphics driver.
// Your version should be > 4.0 for CGT 521 
void printGlInfo()
{
   std::cout << "Vendor: "       << glGetString(GL_VENDOR)                    << std::endl;
   std::cout << "Renderer: "     << glGetString(GL_RENDERER)                  << std::endl;
   std::cout << "Version: "      << glGetString(GL_VERSION)                   << std::endl;
   std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION)  << std::endl;

   int max_locs, max_f_comp, max_v_comp, max_blocks, max_f_block_size, max_v_block_size;
   glGetIntegerv(GL_MAX_UNIFORM_LOCATIONS, &max_locs); //must be at least 1024
												  //max int, float or bool values in uniform storage in fragment shader (at least 1024)
   glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &max_f_comp);
   glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &max_v_comp); //probably same as above
   glGetIntegerv(GL_MAX_VERTEX_UNIFORM_BLOCKS, &max_blocks); //at least 12
   glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS, &max_f_block_size); //at least 12
   glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &max_v_block_size); //at least 16384 bytes
}

void initOpenGl()
{
   //Initialize glew so that new OpenGL function names can be used
   glewInit();

   RegisterCallback();

   glEnable(GL_DEPTH_TEST);

   reload_shaders();

   //Load a mesh and a texture
   for(int i=0; i<4; i++)
   {
      mesh_data[i] = LoadMesh(mesh_name[i]); //Helper function: Uses Open Asset Import library.
   }

   for(int i=0; i<4; i++)
   {
      diffuse_map_id[i] = LoadTexture(diffuse_map_name[i].c_str()); //Helper function: Uses FreeImage library
      normal_map_id[i] = LoadTexture(normal_map_name[i].c_str()); 
      bump_map_id[i] = LoadTexture(bump_map_name[i].c_str()); 
   }

   env_map_id = LoadCube(env_map_name);
}

// glut callbacks need to send keyboard and mouse events to imgui
void keyboard(unsigned char key, int x, int y)
{
   ImGui_ImplGlut_KeyCallback(key);
   std::cout << "key : " << key << ", x: " << x << ", y: " << y << std::endl;

   switch(key)
   {
      case 'r':
      case 'R':
         reload_shaders();     
      break;
   }
}

void keyboard_up(unsigned char key, int x, int y)
{
   ImGui_ImplGlut_KeyUpCallback(key);
}

void special_up(int key, int x, int y)
{
   ImGui_ImplGlut_SpecialUpCallback(key);
}

void passive(int x, int y)
{
   ImGui_ImplGlut_PassiveMouseMotionCallback(x,y);
}

void special(int key, int x, int y)
{
   ImGui_ImplGlut_SpecialCallback(key);
}

void motion(int x, int y)
{
   ImGui_ImplGlut_MouseMotionCallback(x, y);
}

void mouse(int button, int state, int x, int y)
{
   ImGui_ImplGlut_MouseButtonCallback(button, state);
}


int main (int argc, char **argv)
{
   //Configure initial window state using freeglut

   #if _DEBUG
   glutInitContextFlags(GLUT_DEBUG);
   #endif
   glutInitContextVersion(4, 3);

   glutInit(&argc, argv); 
   glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
   glutInitWindowPosition (5, 5);
   glutInitWindowSize (1280, 720);
   int win = glutCreateWindow (argv[0]); //no OpenGL functions before this point

   printGlInfo();

   //Register callback functions with glut. 
   glutDisplayFunc(display); 
   glutKeyboardFunc(keyboard);
   glutSpecialFunc(special);
   glutKeyboardUpFunc(keyboard_up);
   glutSpecialUpFunc(special_up);
   glutMouseFunc(mouse);
   glutMotionFunc(motion);
   glutPassiveMotionFunc(motion);

   glutIdleFunc(idle);

   initOpenGl();
   ImGui_ImplGlut_Init(); // initialize the imgui system

   //Enter the glut event loop.
   glutMainLoop();
   glutDestroyWindow(win);
   return 0;		
}


