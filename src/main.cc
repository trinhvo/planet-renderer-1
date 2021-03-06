#include <stdint.h>
#include <iostream>
#include <Application.h>
#include <RenderManager.h>

#include <Scene.h>
#include <Terrain.h>
#include <Ocean.h>
#include <TextureSkyDome.h>
#include <Grass.h>
#include <Tree.h>
#include "Solar.h"

const float init_phi = M_PI * 0.07;
const float time_factor = (1/90.0);
//const float init_phi = M_PI * 0.45;
//const float time_factor = 1.0;

class PlanetRenderManager : public RenderManager {
	enum ACTION_BIT {
		_CAMERA_LEFT_BIT = 0,
		_CAMERA_RIGHT_BIT,
		_CAMERA_FORWARD_BIT,
		_CAMERA_BACKWARD_BIT,
		_CAMERA_PITCH_UP_BIT,
		_CAMERA_PITCH_DOWN_BIT,
	};
#define DEFINE_ACTION(A)	A = (1 << _##A##_BIT),

	enum : uint64_t {
		DEFINE_ACTION(CAMERA_LEFT)
			DEFINE_ACTION(CAMERA_RIGHT)
			DEFINE_ACTION(CAMERA_FORWARD)
			DEFINE_ACTION(CAMERA_BACKWARD)
			DEFINE_ACTION(CAMERA_PITCH_UP)
			DEFINE_ACTION(CAMERA_PITCH_DOWN)
	};
#undef DEFINE_ACTION

	bool sun_fixed_ = false;
	double init_clock_;
public:

	/**
	 * Notice: do not put your object instantiation code
	 * in the construction. This is due to the fact that
	 * when the constructor is called, the OpenGL context
	 * has not been created, therefore the instantiation
	 * will fail. Instead, please move those codes into 
	 * prepare method.
	 * */
	PlanetRenderManager() : resolution(1024, 768) {
		drawMode = GL_FILL;
		action_flag_ = 0;
	}

	virtual ~PlanetRenderManager() {

	}

	void keyPressed(int count, int key, int modifiers, QString text) {
		switch (key) {
			case Qt::Key_Left:
			case Qt::Key_A:
				action_flag_|= CAMERA_LEFT;
				break;
			case Qt::Key_Right:
			case Qt::Key_D:
				action_flag_|= CAMERA_RIGHT;
				break;
			case Qt::Key_Up:
			case Qt::Key_W:
				action_flag_|= CAMERA_FORWARD;
				break;
			case Qt::Key_Down:
			case Qt::Key_S:
				action_flag_|= CAMERA_BACKWARD;
				break;
			case Qt::Key_PageUp:
				action_flag_|= CAMERA_PITCH_UP;
				break;
			case Qt::Key_PageDown:
				action_flag_|= CAMERA_PITCH_DOWN;
				break;
			case Qt::Key_Space:
				if (drawMode == GL_FILL) { drawMode = GL_LINE; }
				else { drawMode = GL_FILL; }
				reflection_->setDrawMode(drawMode);
				finalPass_->setDrawMode(drawMode);
				break;
		}
	}

	void keyReleased(int count, int key, int modifiers, QString text)
	{
		switch (key) {
			case Qt::Key_Left:
			case Qt::Key_A:
				action_flag_ &= ~CAMERA_LEFT;
				break;
			case Qt::Key_Right:
			case Qt::Key_D:
				action_flag_ &= ~CAMERA_RIGHT;
				break;
			case Qt::Key_Up:
			case Qt::Key_W:
				action_flag_ &= ~CAMERA_FORWARD;
				break;
			case Qt::Key_Down:
			case Qt::Key_S:
				action_flag_ &= ~CAMERA_BACKWARD;
				break;
			case Qt::Key_PageUp:
				action_flag_ &= ~CAMERA_PITCH_UP;
				break;
			case Qt::Key_PageDown:
				action_flag_ &= ~CAMERA_PITCH_DOWN;
				break;
		}
	}

	void mouseDragStarted(int x, int y) {
		dragStartX = x;
		dragStartY = y;
	}

	void mouseDragging(int x, int y) {
		int diffX = x - dragStartX;
		int diffY = y - dragStartY;

		dragStartX = x;
		dragStartY = y;

		double angleX = 90 * diffX / (resolution.width() / 2.0);
		camera_->turnRight(angleX);

		double angleY = 90 * diffY / (resolution.height() / 2.0);
		camera_->lookUp(angleY);
	}

	void see_sun()
	{
		QVector3D sunpos = camera_->getPosition() + solar_->get_pos();
		sunpos += QVector3D(0.0, -1600.0, 0.0);
		camera_->lookAt(camera_->getPosition(),
				sunpos,
				QVector3D(0.0, 1.0, 0.1));
	}

	void sun_tweak_fixed()
	{
		sun_fixed_ = !sun_fixed_;
	}

	void prepare() {
		int width = resolution.width();
		int height = resolution.height();

		solar_ = new Solar("Solar",
				0.0 * M_PI,
				init_phi,
				100.0,
				{0.1, 0.1, 0.1},
				{1.1, 1.1, 1.1},
				{1.0, 1.0, 1.0});

		// Create camera
		camera_.reset(new Camera("Viewer Camera"));
		camera_->setPerspective(45.0, (float)width/(float)height, 1.0, 10000.0);
		reflectCamera_.reset(new Camera("Reflection Camera"));
		reflectCamera_->setPerspective(45.0, (float)width/(float)height, 1.0, 10000.0);

		// For terrain navigation
		float cc = 524288.0f;
		camera_->lookAt(
				QVector3D(cc, 300.0, cc),
				//QVector3D(cc, 150.0, cc+100.0),
				solar_->get_pos(),
				QVector3D(0.0, 1.0, 0.0)
			       );

		// Create two passes
		reflection_.reset(new Scene("reflection", resolution));

		finalPass_.reset(new Scene("final result", resolution));
		finalPass_->setCamera(camera_.get());

		// Instantiate scene objects
		skydome_.reset(new TextureSkyDome(64, finalPass_.get()));
		terrain_.reset(new Terrain(32, 5, finalPass_.get()));
		ocean_.reset(new Ocean(32, 5, finalPass_.get()));

		// grassFactory_.reset(new GrassFactory(terrain_.get()));
		// grass_.reset(grassFactory_->createGrass(QVector2D(cc+1000, cc+1000), 10000.0, 10.0, 80.0, 40.0, 68435, 0.125));

		treeFactory_.reset(new TreeFactory(terrain_.get()));
		tree1_.reset(treeFactory_->createTree(TreeType::PALM, QVector2D(cc+2000, cc+2000), 10000.0, 200.0, 140.0, 200.0, 3389, 0.25));
		tree2_.reset(treeFactory_->createTree(TreeType::TREE1, QVector2D(cc+3100, cc+3100), 10000.0, 200.0, 140.0, 200.0, 9527, 0.25));

		// Adding objects into reflection pass
		reflection_->add_light(solar_);
		reflection_->addObject(skydome_.get());
		reflection_->addObject(terrain_.get());
		// reflection_->addObject(grass_.get());
		reflection_->addObject(tree1_.get());
		reflection_->addObject(tree2_.get());

		// Adding objects into final pass
		finalPass_->add_light(solar_);
		finalPass_->addObject(skydome_.get());
		finalPass_->addObject(terrain_.get());
		finalPass_->addObject(ocean_.get());
		// finalPass_->addObject(grass_.get());
		finalPass_->addObject(tree1_.get());
		finalPass_->addObject(tree2_.get());

		// setContextProperty("fps", "0");

		// OpenGL settings
		glEnable(GL_DEPTH_TEST);

		glEnable(GL_POLYGON_SMOOTH);
		glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

		init_clock_ = clock();
	}

	void do_camera_move()
	{
		double interval = getInterval();

		if (action_flag_ & CAMERA_LEFT) {
			camera_->turnLeft(1);
		}
		if (action_flag_ & CAMERA_RIGHT) {
			camera_->turnRight(1);
		}
		if (action_flag_ & CAMERA_FORWARD) {
			camera_->moveForward(500 * interval);
		}
		if (action_flag_ & CAMERA_BACKWARD) {
			camera_->moveBackward(500 * interval);
		}
		if (action_flag_ & CAMERA_PITCH_UP) {
			camera_->lookUp(5);
		}
		if (action_flag_ & CAMERA_PITCH_DOWN) {
			camera_->lookDown(5);
		}

	}

	void render(QOpenGLFramebufferObject *fbo) {

		// qDebug() << fps();
		// setContextProperty("fps", QString::number(fps()));

		do_camera_move();

		if (sun_fixed_) {
			solar_->update_polar(0.0 * M_PI,
					0.45 * M_PI);
		} else {
			double relsec = (double(clock()) - init_clock_)/CLOCKS_PER_SEC;
			solar_->update_polar(0.0 * M_PI,
					init_phi + relsec * time_factor * M_PI);
		}
#if 0
		fprintf(stderr, "Solar (%f, %f, %f)\n", 
				solar_->get_pos().x(),
				solar_->get_pos().y(),
				solar_->get_pos().z());
#endif

		terrain_->underWaterCulling(QVector4D(0.0, 1.0, 0.0, 0.0));
		if (camera_->getPosition().y() >= 0) {
			// reflection
			camera_->reflectCamera(QVector4D(0.0, 1.0, 0.0, 0.0), reflectCamera_.get());
			reflection_->setCamera(reflectCamera_.get());
			reflection_->set_center(camera_->getPosition());
		} else {
			// refraction
			reflection_->setCamera(camera_.get());
		}
		reflection_->renderScene();

		terrain_->disableUnderWaterCulling();
		finalPass_->useTexture(reflection_->texture());
		finalPass_->setCamera(camera_.get());
		finalPass_->renderScene(fbo);
	}

	void shutdown() {
		/*
 		 * Release GL resource here since we need GL context to do
		 * this.
		 */
		// Reset scenes
		reflection_.reset();
		finalPass_.reset();

		// Reset scene objects
		skydome_.reset();
		terrain_.reset();
		grassFactory_.reset();
		treeFactory_.reset();

		grass_.reset();
		tree1_.reset();
		tree2_.reset();

#if 0 // No, we don't need to release normal objects.
		camera_.reset();
		reflectCamera_.reset();
#endif
	}

private:
	QSize resolution;
	GLuint drawMode;

	// Render Targets
	std::unique_ptr<Scene> reflection_;
	// std::unique_ptr<Scene> shadow_;
	std::unique_ptr<Scene> finalPass_;

	// Scene Objects
	std::unique_ptr<Terrain> terrain_;
	std::unique_ptr<Ocean> ocean_;
	std::unique_ptr<TextureSkyDome> skydome_;
	std::unique_ptr<GrassFactory> grassFactory_;
	std::unique_ptr<Grass> grass_;
	std::unique_ptr<TreeFactory> treeFactory_;
	std::unique_ptr<Tree> tree1_;
	std::unique_ptr<Tree> tree2_;

	// Camera
	std::unique_ptr<Camera> camera_;
	std::unique_ptr<Camera> reflectCamera_;
	Solar* solar_ = nullptr;
	uint64_t action_flag_;

	// Mouse movement
	int dragStartX;
	int dragStartY;
};


int main(int argc, char *argv[])
{
	/**
	 * Sugguested framework:
	 * 1. Create a Application wrapper, where common
	 * stuff are managed, such as shaders containing common
	 * functions, render managers.
	 *
	 * 2. Create a render manager, which is used to render
	 * customized objects with render targets. The render
	 * manager should be a singleton all over the application.
	 * In the render manager, we can customize the rendering
	 * sequence of multiple passes rendering.
	 *
	 * 3. Register the render manager in application so that 
	 * the render thread will use render manager as the rendering
	 * engine.
	 * */

	Application app("PlanetRenderer", argc, argv);

	app.registerRenderManager(new PlanetRenderManager());

	return app.exec();
}
