/* ResidualVM - A 3D game interpreter
 *
 * ResidualVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the AUTHORS
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "engines/myst3/cursor.h"
#include "engines/myst3/directorysubentry.h"
#include "engines/myst3/myst3.h"
#include "engines/myst3/scene.h"
#include "engines/myst3/state.h"

#include "graphics/surface.h"
#include "image/bmp.h"

namespace Myst3 {

struct CursorData {
	uint32 nodeID;
	uint16 hotspotX;
	uint16 hotspotY;
	Texture *texture;
	double transparency;
};

static CursorData availableCursors[13] = {
	{ 1000,  8,  8, 0, 0.25 },
	{ 1001,  8,  8, 0, 0.5  },
	{ 1002,  8,  8, 0, 0.5  },
	{ 1003,  1,  5, 0, 0.5  },
	{ 1004, 14,  5, 0, 0.5  },
	{ 1005, 16, 14, 0, 0.5  },
	{ 1006, 16, 14, 0, 0.5  },
	{ 1007,  8,  8, 0, 0.55 },
	{ 1000,  8,  8, 0, 0.25 },
	{ 1001,  8,  8, 0, 0.5  },
	{ 1011, 16, 16, 0, 0.5  },
	{ 1000,  6,  1, 0, 0.5  },
	{    0,  0,  0, 0, 0    }
};

Cursor::Cursor(Myst3Engine *vm) :
	_vm(vm),
	_position(vm->_gfx->frameCenter()),
	_hideLevel(0),
	_lockedAtCenter(false) {

	// Load available cursors
	loadAvailableCursors();

	// Set default cursor
	changeCursor(8);
}

void Cursor::loadAvailableCursors() {
	// Load available cursors
	for (uint i = 0; availableCursors[i].nodeID; i++) {
		const DirectorySubEntry *cursorDesc = _vm->getFileDescription("GLOB", availableCursors[i].nodeID, 0, DirectorySubEntry::kRawData);

		if (!cursorDesc)
			error("Cursor %d does not exist", availableCursors[i].nodeID);

		Common::MemoryReadStream *bmpStream = cursorDesc->getData();

		Image::BitmapDecoder bitmapDecoder;
		if (!bitmapDecoder.loadStream(*bmpStream))
			error("Could not decode Myst III bitmap");
		const Graphics::Surface *surfaceBGRA = bitmapDecoder.getSurface();
		Graphics::Surface *surfaceRGBA = surfaceBGRA->convertTo(Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24));

		delete bmpStream;

		// Apply the colorkey for transparency
		for (uint u = 0; u < surfaceRGBA->w; u++) {
			for (uint v = 0; v < surfaceRGBA->h; v++) {
				uint32 *pixel = (uint32*)(surfaceRGBA->getBasePtr(u, v));
				if (*pixel == 0xFF00FF00)
					*pixel = 0x0000FF00;

			}
		}

		availableCursors[i].texture = _vm->_gfx->createTexture(surfaceRGBA);
		surfaceRGBA->free();
		delete surfaceRGBA;
	}
}

Cursor::~Cursor() {
	// Free available cursors
	for (uint i = 0; availableCursors[i].nodeID; i++) {
		if (availableCursors[i].texture) {
			_vm->_gfx->freeTexture(availableCursors[i].texture);
			availableCursors[i].texture = 0;
		}
	}
}

void Cursor::changeCursor(uint32 index) {
	if (index > 12)
		return;

	_currentCursorID = index;
}

void Cursor::lockPosition(bool lock) {
	if (_lockedAtCenter == lock)
		return;

	_lockedAtCenter = lock;

	g_system->lockMouse(lock);

	Common::Point center = _vm->_gfx->frameCenter();
	if (_lockedAtCenter) {
		// Locking, just mouve the cursor at the center of the screen
		_position = center;
	} else {
		// Unlocking, warp the actual mouse position to the cursor
		g_system->warpMouse(center.x, center.y);
	}
}

void Cursor::updatePosition(Common::Point &mouse) {
	if (!_lockedAtCenter) {
		_position = mouse;
	}
}

Common::Point Cursor::getPosition() {
	Common::Rect viewport = _vm->_gfx->viewport();

	// The rest of the engine expects 640x480 coordinates
	Common::Point scaledPosition = _position;
	scaledPosition.x -= viewport.left;
	scaledPosition.y -= viewport.top;
	scaledPosition.x = CLIP<int16>(scaledPosition.x, 0, viewport.width());
	scaledPosition.y = CLIP<int16>(scaledPosition.y, 0, viewport.height());
	scaledPosition.x *= Renderer::kOriginalWidth / (float)viewport.width();
	scaledPosition.y *= Renderer::kOriginalHeight / (float)viewport.height();

	return scaledPosition;
}

void Cursor::draw() {
	CursorData &cursor = availableCursors[_currentCursorID];

	// Rect where to draw the cursor
	Common::Rect screenRect = Common::Rect(cursor.texture->width, cursor.texture->height);
	screenRect.translate(_position.x - cursor.hotspotX, _position.y - cursor.hotspotY);

	// Rect where to draw the cursor
	Common::Rect textureRect = Common::Rect(cursor.texture->width, cursor.texture->height);

	float transparency = 1.0;

	int32 varTransparency = _vm->_state->getCursorTransparency();
	if (_lockedAtCenter || varTransparency == 0) {
		if (varTransparency >= 0)
			transparency = varTransparency / 100.0;
		else
			transparency = cursor.transparency;
	}

	_vm->_gfx->drawTexturedRect2D(screenRect, textureRect, cursor.texture, transparency);
}

void Cursor::setVisible(bool show) {
	if (show)
		_hideLevel = MAX<int32>(0, --_hideLevel);
	else
		_hideLevel++;
}

bool Cursor::isVisible() {
	return !_hideLevel && !_vm->_state->getCursorHidden() && !_vm->_state->getCursorLocked();
}

void Cursor::getDirection(float &pitch, float &heading) {
	if (_lockedAtCenter) {
		pitch = _vm->_state->getLookAtPitch();
		heading = _vm->_state->getLookAtHeading();
	} else {
		_vm->_gfx->screenPosToDirection(_position, pitch, heading);
	}
}

} // End of namespace Myst3
