#include "framework.h"
#include "image.h"
#include "Timer.h"
#include "vfxNodes/dotDetector.h"
#include "../avpaint/video.h"

#define USE_READPIXELS_OPTIMIZE 1
#define USE_READPIXELS_FENCES 0

extern const int GFX_SX;
extern const int GFX_SY;

void testDotDetector()
{
	const int sx = 768;
	const int sy = 512;
	Surface surface(sx, sy, false, false, SURFACE_R8);
	
	// make sure the surface turns up black and white instead of shades of red when we draw it, by applying a swizzle mask
	glBindTexture(GL_TEXTURE_2D, surface.getTexture());
	GLint swizzleMask[4] = { GL_RED, GL_RED, GL_RED, GL_ONE };
	glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
	checkErrorGL();
	
	uint8_t * surfaceData = new uint8_t[sx * sy];
	memset(surfaceData, 0xff, sizeof(uint8_t) * sx * sy);
	
	uint8_t * maskedData = new uint8_t[sx * sy];
	
	struct Circle
	{
		float x;
		float y;
		float angle;
		float speed;
		float timer;
		float timerRcp;
		
		void randomize()
		{
			x = random(0, sx);
			y = random(0, sy);
			angle = random(0.f, float(M_PI * 2.f));
			speed = random(50.f, 200.f);
			timer = random(1.f, 3.f);
			timerRcp = 1.f / timer;
		}
		
		void tick(const float dt)
		{
			const float dx = std::cosf(angle);
			const float dy = std::sinf(angle);
			
			x += dx * speed * dt;
			y += dy * speed * dt;
			
			timer -= framework.timeStep;
			
			if (timer <= 0.f)
			{
				randomize();
			}
		}
	};
	
	const int kNumCircles = 100;
	Circle circles[kNumCircles];
	
	for (auto & c : circles)
	{
		c.randomize();
	}
	
	bool useVideo = true;
	
	MediaPlayer mp;
	
	if (useVideo)
	{
		mp.openAsync("mocapc.mp4", MP::kOutputMode_PlanarYUV);
	}
	
	//
	
	uint64_t averageTime = 0;
	uint64_t averageTimeR = 0;
	uint64_t averageTimeM = 0;
	
	bool useGrid = true;
	int tresholdFunction = 1;
	int tresholdValue = 32;
	int maxRadius = 10;
	
#if USE_READPIXELS_OPTIMIZE
	const int kNumPixelBuffers = 2;
	bool hasPixels = false;
	int pixelBufferIndex = 0;
	GLuint pixelBuffers[kNumPixelBuffers] = { };
#if USE_READPIXELS_FENCES
	GLsync pixelSyncs[kNumPixelBuffers] = { };
#endif
	
	for (int i = 0; i < kNumPixelBuffers; ++i)
	{
		glGenBuffers(1, &pixelBuffers[i]);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, pixelBuffers[i]);
		glBufferData(GL_PIXEL_PACK_BUFFER, sx * sy, 0, GL_DYNAMIC_READ);
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		checkErrorGL();
	}
#endif

	do
	{
		framework.process();
		
		//
		
		if (keyboard.wentDown(SDLK_g))
			useGrid = !useGrid;
		if (keyboard.wentDown(SDLK_t))
			tresholdFunction = (tresholdFunction + 1) % 2;
		if (keyboard.wentDown(SDLK_UP, true))
			tresholdValue += 1;
		if (keyboard.wentDown(SDLK_DOWN, true))
			tresholdValue -= 1;
		if (keyboard.wentDown(SDLK_a, true))
			maxRadius += 1;
		if (keyboard.wentDown(SDLK_z, true) && maxRadius > 1)
			maxRadius -= 1;
		
		//
		
		const float dt = framework.timeStep * (mouse.isDown(BUTTON_LEFT) ? mouse.y / float(GFX_SY) * 4.f : 1.f);
		
		//
		
		if (mp.isActive(mp.context))
		{
			mp.presentTime += dt;
			
			mp.tick(mp.context, true);
			
			if (mp.context->hasPresentedLastFrame)
			{
				const std::string filename = mp.context->openParams.filename;
				
				mp.close(false);
				
				mp.presentTime = 0.0;
				
				mp.openAsync(filename.c_str(), MP::kOutputMode_RGBA);
			}
		}
		
		// generate a new pattern of moving dots
		
		uint64_t tr1;
		uint64_t tr2;
		
		pushSurface(&surface);
		{
			surface.clear(255, 255, 255, 255);
			
			if (useVideo)
			{
				if (mp.getTexture())
				{
					pushBlend(BLEND_OPAQUE);
					gxSetTexture(mp.getTexture());
					setColor(colorWhite);
					drawRect(0, 0, sx, sy);
					gxSetTexture(0);
					popBlend();
				}
			}
			else
			{
				pushBlend(BLEND_ALPHA);
				hqBegin(HQ_FILLED_CIRCLES);
				{
					for (auto & c : circles)
					{
						const float radius = c.timer * c.timerRcp * 12.f + 3.f;
						
						setColor(colorBlack);
						hqFillCircle(c.x, c.y, radius);
						
						c.tick(dt);
					}
				}
				hqEnd();
				popBlend();
			}
			
			// capture the dots image into cpu accessible memory
			// note : this is slow unless the operation is delayed by 1 or 2 frames,
			//        but for testing purposes we don't care so much about performance
			
			tr1 = g_TimerRT.TimeUS_get();
			
		#if USE_READPIXELS_OPTIMIZE
			glReadBuffer(GL_COLOR_ATTACHMENT0);
			glBindBuffer(GL_PIXEL_PACK_BUFFER, pixelBuffers[pixelBufferIndex]);
			//logDebug("readPixels %d", pixelBufferIndex);
			glReadPixels(0, 0, sx, sy, GL_RED, GL_UNSIGNED_BYTE, 0);
			glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
			checkErrorGL();
			
		#if USE_READPIXELS_FENCES
			Assert(pixelSyncs[pixelBufferIndex] == 0);
			pixelSyncs[pixelBufferIndex] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
			checkErrorGL();
		#endif
			
			pixelBufferIndex = (pixelBufferIndex + 1) % kNumPixelBuffers;
		#else
			glReadPixels(0, 0, sx, sy, GL_RED, GL_UNSIGNED_BYTE, surfaceData);
		#endif
			
			tr2 = g_TimerRT.TimeUS_get();
		}
		popSurface();
		
		averageTimeR = ((tr2 - tr1) * 1 + averageTimeR * 49) / 50;
		
	#if USE_READPIXELS_OPTIMIZE
		if (pixelBufferIndex == 0)
			hasPixels = true;
		
		if (hasPixels)
	#endif
		{
			const uint64_t tm1 = g_TimerRT.TimeUS_get();
			
		#if USE_READPIXELS_OPTIMIZE
		#if USE_READPIXELS_FENCES
			Assert(pixelSyncs[pixelBufferIndex] != 0);
			const GLenum syncState = glClientWaitSync(pixelSyncs[pixelBufferIndex], GL_SYNC_FLUSH_COMMANDS_BIT, 0);
			checkErrorGL();
			
			Assert(syncState != GL_TIMEOUT_EXPIRED);
			if (syncState != GL_TIMEOUT_EXPIRED)
			{
				Assert(syncState == GL_ALREADY_SIGNALED || syncState == GL_CONDITION_SATISFIED);
				pixelSyncs[pixelBufferIndex] = 0;
			}
			else
			{
				const GLenum syncState = glClientWaitSync(pixelSyncs[pixelBufferIndex], GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
				checkErrorGL();
				
				Assert(syncState == GL_ALREADY_SIGNALED || syncState == GL_CONDITION_SATISFIED);
				pixelSyncs[pixelBufferIndex] = 0;
			}
		#endif
		
			glBindBuffer(GL_PIXEL_PACK_BUFFER, pixelBuffers[pixelBufferIndex]);
			//logDebug("mapBuffer %d", pixelBufferIndex);
			const uint8_t * __restrict surfaceData = (uint8_t*)glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, sx * sy, GL_MAP_READ_BIT);
			checkErrorGL();
		#endif
			
			const int treshold = tresholdFunction == 0 ? 255 - tresholdValue : tresholdValue;
			const DotDetector::TresholdTest test = tresholdFunction == 0 ? DotDetector::kTresholdTest_GreaterEqual : DotDetector::kTresholdTest_LessEqual;
			
			DotDetector::treshold(surfaceData, sx, maskedData, sx, sx, sy, test, treshold);
			
		#if USE_READPIXELS_OPTIMIZE
			glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
			glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
			checkErrorGL();
		#endif
			
			const uint64_t tm2 = g_TimerRT.TimeUS_get();
		
			averageTimeM = ((tm2 - tm1) * 1 + averageTimeM * 49) / 50;
		}
		
		// detect dots
		
		const int kMaxIslands = 1024;
		DotIsland islands[kMaxIslands];
		
		const int radius = mouse.isDown(BUTTON_LEFT) ? (1 + mouse.x / 30) : maxRadius;
		
		const uint64_t td1 = g_TimerRT.TimeUS_get();
		
		const int numIslands = DotDetector::detectDots(maskedData, sx, sy, radius, islands, kMaxIslands, useGrid);
		
		const uint64_t td2 = g_TimerRT.TimeUS_get();
		
		// update the smoothed out over time running average
		
		averageTime = ((td2 - td1) * 1 + averageTime * 49) / 50;
		
		// visualize the dot detection results!
		
		framework.beginDraw(0, 0, 0, 0);
		{
			// draw the original pattern of moving dots
			
			pushBlend(BLEND_OPAQUE);
			setColor(colorWhite);
			gxSetTexture(surface.getTexture());
			drawRect(0, 0, sx, sy);
			gxSetTexture(0);
			popBlend();
			
			// draw detected dots
			
			hqBegin(HQ_STROKED_CIRCLES);
			{
				for (int i = 0; i < numIslands; ++i)
				{
					auto & island = islands[i];
					
					const int dx = island.maxX - island.minX;
					const int dy = island.maxY - island.minY;
					const float ds = std::sqrtf(dx * dx + dy * dy) / 2.f + 1.f;
					
					setColor(colorRed);
					hqStrokeCircle(island.x, island.y, radius, 2.f);
					
					setColor(colorGreen);
					hqStrokeCircle(island.x, island.y, ds, 2.f);
				}
			}
			hqEnd();
			
 			// draw stats and instructional text
			
			setFont("calibri.ttf");
			setColor(colorGreen);
			
			int y = 5 + sy;
			int fontSize = 12;
			int spacing = fontSize + 4;
			
			drawText(5, y, fontSize, +1, +1, "detected %d dots. process took %.02fms, average %.02fms", numIslands, (td2 - td1) / 1000.f, averageTime / 1000.f);
			y += spacing;
			drawText(5, y, fontSize, +1, +1, "opengl-read took %.02fms, treshold took %.02fms", averageTimeR / 1000.f, averageTimeM / 1000.f);
			y += spacing;
			drawText(5, y, fontSize, +1, +1, "useGrid: %d, tresholdFunction: %d, tresholdValue: %d", useGrid ? 1 : 0, tresholdFunction, tresholdValue);
			y += spacing;
			drawText(5, y, fontSize, +1, +1, "G = toggle grid. T = next treshold function. A/Z = change treshold");
			y += spacing;
			drawText(5, y, fontSize, +1, +1, "SPACE = quit test. MOUSE_LBUTTON = enable speed/radius test");
			y += spacing;
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_SPACE));
	
#if USE_READPIXELS_OPTIMIZE
	for (int i = 0; i < kNumPixelBuffers; ++i)
	{
		glDeleteBuffers(1, &pixelBuffers[i]);
		pixelBuffers[i] = 0;
		checkErrorGL();
		
	#if USE_READPIXELS_FENCES
		if (pixelSyncs[i] != 0)
		{
			const GLenum syncState = glClientWaitSync(pixelSyncs[pixelBufferIndex], GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
			Assert(syncState != GL_TIMEOUT_EXPIRED);
			checkErrorGL();
		}
	#endif
	}
#endif
	
	mp.close(true);
	
	delete[] maskedData;
	maskedData = nullptr;

	delete[] surfaceData;
	surfaceData = nullptr;
}
