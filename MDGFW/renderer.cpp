#include <MDGFW/renderer.h>

Renderer::Renderer()
{
	_window_width = MINSWIDTH;
	_window_height = MINSHEIGHT;
	this->init();
}

Renderer::~Renderer()
{

}

int Renderer::init()
{
	// Initialise GLFW
	if ( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		return -1;
	}

	glfwWindowHint( GLFW_SAMPLES, 4 );
	glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 2 );
	glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
	glfwWindowHint( GLFW_RESIZABLE, false );//Can the user resize the window or not?

	GLFWmonitor* primaryMonitor = nullptr;

	if ( FULLSCREEN ) {
		primaryMonitor = glfwGetPrimaryMonitor();
	}

	// Open a window and create its OpenGL context
	_window = glfwCreateWindow( _window_width, _window_height, TITLE, primaryMonitor, nullptr );
	if ( _window == NULL ) {
		fprintf( stderr, "Failed to open GLFW window.\n" );
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent( _window );

	glewExperimental = GL_TRUE;

	// Initialize GLEW
	if ( glewInit() != GLEW_OK ) {
		fprintf( stderr, "Failed to initialize GLEW\n" );
		return -1;
	}

	glViewport( 0, 0, _window_width, _window_height );

	ResourceManager::getManager();//Calling it ones for instantiating

	useVSYNC( VSYNC );

	//Use when user is able to resize (this restricts the user's window size between 2 values on the x and y (WINDOW,MINX,MINY,MAXX,MAXY))
	//glfwSetWindowSizeLimits( _window, MINSWIDTH, MINSHEIGHT, MINSWIDTH, MINSHEIGHT );

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(_window, GLFW_STICKY_KEYS, GL_TRUE);

	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

	// Enable depth test
	//glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	//glDepthFunc(GL_LESS);

	// Cull triangles which normal is not towards the camera
	glEnable( GL_CULL_FACE );

	// Blend textures so alpha channels work.
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	_camera = new Camera();
	return 0;
}

void Renderer::updateWorld( Scene* world ) {

	// Clear the screen
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// Compute the ViewMatrix from keyboard and mouse input (see: camera.h/cpp)
	_camera->updateCamera(_window);
	//glm::vec3 cursor = getCursor(); // from Camera
	//printf("(%f,%f)\n",cursor.x, cursor.y);

	glm::mat4 modelMatrix = glm::mat4( 1.0f );
	renderEntity( world, modelMatrix);

	// Swap buffers
	glfwSwapBuffers( _window );
}

void Renderer::renderEntity( Entity* entity, glm::mat4 modelMatrix) {

	entity->update();

	glm::mat4 projectionMatrix = _camera->getProjectionMatrix(); // get from Camera (Camera mode (2D/3D | Orthographic/Perspective))
	glm::mat4 viewMatrix = _camera->getViewMatrix(); // get from Camera (Camera position and direction)

	Vector3 localPos = entity->position;
	Vector3 localRot = entity->rotation;
	Vector3 localScl = entity->scale;

	// Build the Model matrix
	glm::mat4 translationMatrix = glm::translate( glm::mat4( 1.0f ), localPos.toVec3() );
	glm::mat4 rotationMatrix = glm::eulerAngleYXZ( localRot.x, localRot.y, localRot.z );
	glm::mat4 scalingMatrix = glm::scale( glm::mat4( 1.0f ), localScl.toVec3() );
	
	glm::mat4 mM = translationMatrix * rotationMatrix * scalingMatrix;

	modelMatrix *= mM;

	glm::mat4 MVP = projectionMatrix * viewMatrix * modelMatrix;

	glm::vec3 realscale;
	glm::quat realrot;
	glm::vec3 realpos;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose( modelMatrix, realscale, realrot, realpos, skew, perspective );

	Sprite* sprite = entity->getSprite();
	if ( sprite != nullptr ) {
		renderSprite( sprite->getShader(), sprite, MVP );
	}

	renderLines( entity, Vector3(realpos));

	for each (Text* text in entity->getTexts()) {
		RenderText( text, MVP);
	}

	if ( sprite != nullptr ) {
		renderSprite( sprite->getShader(), sprite, MVP );
	}

	for each (Entity* child in entity->getChilds()) {
		renderEntity( child, modelMatrix);
	}
}

void Renderer::renderLines( Entity* entity, Vector3 worldPos) {
	//TODO Create line rendering implementation
}

void Renderer::renderSprite( Shader* shader, Sprite* sprite, glm::mat4 MVP )
{
	shader->use();

	setObjectTransform( shader, MVP );

	setCurrentTexture( shader, sprite->getTexture() );

	this->renderMesh(sprite->getShader(), sprite->getMesh(), GL_TRIANGLES );
}

void Renderer::setCurrentTexture( Shader* shader, Texture* texture ) {
	if ( shader->isActive() ) {
		// Bind our texture in Texture Unit 0
		glActiveTexture( GL_TEXTURE0 );
		glBindTexture( GL_TEXTURE_2D, texture->getID() );
		// Set our "myTextureSampler" sampler to user Texture Unit 0
		GLuint textureID = glGetUniformLocation( shader->getID(), "myTextureSampler" );
		glUniform1i( textureID, 0 );
	}
}

void Renderer::setObjectTransform( Shader* shader, glm::mat4 MVP) {
	if ( shader->isActive() ) {
		// Send our transformation to the currently bound shader,
		// in the "MVP" uniform
		GLuint matrixID = glGetUniformLocation( shader->getID(), "MVP" );
		glUniformMatrix4fv( matrixID, 1, GL_FALSE, &MVP[0][0] );
	}
}

void Renderer::RenderText( Text* text, glm::mat4 MVP )
{
	Vector2 pos = text->getCanvasPosition();

	GLfloat x = pos.x;
	GLfloat y = pos.y;
	GLfloat scale = text->getScale();
	RGBAColor color = text->getColor();
	Mesh* mesh = text->getMesh();

	// Activate corresponding render state	
	text->getShader()->use();
	glUniform3f( glGetUniformLocation( text->getShader()->getID(), "textColor" ), color.r, color.g, color.b );
	glActiveTexture( GL_TEXTURE0 );
	glBindVertexArray( text->getVAO() );

	std::string message = text->getMessage();

	std::map<GLchar, Character> Characters = text->getCharacters();

	// Iterate through all characters
	std::string::const_iterator c;
	for ( c = message.begin(); c != message.end(); c++ )
	{
		Character ch = Characters[*c];

		GLfloat xpos = x + ch.Bearing.x * scale;
		GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

		GLfloat w = ch.Size.x * scale;
		GLfloat h = ch.Size.y * scale;
		// Update VBO for each character
		GLfloat vertices[6][4] = {
			{ xpos,     ypos + h,   0.0, 0.0 },
			{ xpos,     ypos,       0.0, 1.0 },
			{ xpos + w, ypos,       1.0, 1.0 },

			{ xpos,     ypos + h,   0.0, 0.0 },
			{ xpos + w, ypos,       1.0, 1.0 },
			{ xpos + w, ypos + h,   1.0, 0.0 }
		};
		// Render glyph texture over quad
		glBindTexture( GL_TEXTURE_2D, ch.TextureID );
		// Update content of VBO memory
		glBindBuffer( GL_ARRAY_BUFFER, text->getVBO() );
		glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof( vertices ), vertices ); // Be sure to use glBufferSubData and not glBufferData

		glBindBuffer( GL_ARRAY_BUFFER, 0 );
		// Render quad
		glDrawArrays( GL_TRIANGLES, 0, 6 );
		// Now advance cursors for next glyph (note that advance is number of 1/64 pixels)
		x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
	}
	glBindVertexArray( 0 );
	glBindTexture( GL_TEXTURE_2D, 0 );
}

void Renderer::renderMesh(Shader* shader, Mesh* mesh, GLuint mode) {
	// 1st attribute buffer : vertices
	GLuint vertexPosition_modelspaceID = glGetAttribLocation( shader->getID(), "vertexPosition_modelspace" );
	glEnableVertexAttribArray( vertexPosition_modelspaceID );
	glBindBuffer( GL_ARRAY_BUFFER, mesh->getVertexBuffer() );
	glVertexAttribPointer(
		vertexPosition_modelspaceID,  // The attribute we want to configure
		3,							// size : x+y+z => 3
		GL_FLOAT,					 // type
		GL_FALSE,					 // normalized?
		0,							// stride
		( void* ) 0					  // array buffer offset
	);

	// 2nd attribute buffer : UVs
	GLuint vertexUVID = glGetAttribLocation( shader->getID(), "vertexUV" );
	glEnableVertexAttribArray( vertexUVID );
	glBindBuffer( GL_ARRAY_BUFFER, mesh->getUVBuffer() );
	glVertexAttribPointer(
		vertexUVID,				   // The attribute we want to configure
		2,							// size : U+V => 2
		GL_FLOAT,					 // type
		GL_FALSE,					 // normalized?
		0,							// stride
		( void* ) 0					  // array buffer offset
	);

	// Draw the triangles!
	glDrawArrays( mode, 0, mesh->getVertices() );

	glDisableVertexAttribArray( vertexPosition_modelspaceID );
	glDisableVertexAttribArray( vertexUVID );
}