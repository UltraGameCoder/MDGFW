#ifndef RENDERER_H
#define RENDERER_H

#include <iostream>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>

#include <MDGFW\Mathmatics.h>
#include <MDGFW/camera.h>
#include <MDGFW/Line.h>
#include <MDGFW/sprite.h>
#include <MDGFW/Scene.h>
#include <MDGFW/SceneManager.h>

class Renderer
{
	public:
		Renderer();
		virtual ~Renderer();
		void updateWorld( Scene* world );

		GLFWwindow* window() { return _window; };

		unsigned int width() { return _window_width; };
		unsigned int height() { return _window_height; };

		void useVSYNC(bool state) {
			glfwSwapInterval( state );
		}

	private:

		Camera* _camera;

		int init();

		GLFWwindow* _window;
		unsigned int _window_width;
		unsigned int _window_height;

		void Renderer::setObjectTransform( Shader* shader, glm::mat4 MVP );
		void Renderer::setCurrentTexture( Shader* shader, Texture* texture );

		void renderLines( Entity* entity, Vector3 worldPos);
		void renderEntity( Entity* entity, glm::mat4 modelMatrix);
		void renderSprite( Shader* shader, Sprite* sprite, glm::mat4 MVP );
		void RenderText( Text* text, glm::mat4 MVP );

		void renderMesh( Shader* shader, Mesh* mesh, GLuint mode );
};

#endif /* RENDERER_H */
