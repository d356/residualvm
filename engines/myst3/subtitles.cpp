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

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "engines/myst3/subtitles.h"
#include "engines/myst3/myst3.h"
#include "engines/myst3/state.h"

#include "graphics/fontman.h"
#include "graphics/font.h"

namespace Myst3 {

Subtitles::Subtitles(Myst3Engine *vm, uint32 id) :
	_vm(vm),
	_id(id),
	_surface(0),
	_texture(0),
	_frame(-1) {

	// Subtitles may be overridden using a variable
	const DirectorySubEntry *desc;
	if (_vm->_state->getMovieOverrideSubtitles()) {
		id = _vm->_state->getMovieOverrideSubtitles();
		_vm->_state->setMovieOverrideSubtitles(0);

		desc = _vm->getFileDescription("IMGR", 100000 + id, 0, DirectorySubEntry::kText);
	} else {
		desc = _vm->getFileDescription(0, 100000 + id, 0, DirectorySubEntry::kText);
	}

	if (!desc)
		error("Subtitles %d don't exist", 100000 + id);

	Common::MemoryReadStream *crypted = desc->getData();

	// Read the frames and associated text offsets
	while (true) {
		Phrase s;
		s.frame = crypted->readUint32LE();
		s.offset = crypted->readUint32LE();

		if (!s.frame)
			break;

		_phrases.push_back(s);
	}

	// Read and decrypt the frames subtitles
	for (uint i = 0; i < _phrases.size(); i++) {
		crypted->seek(_phrases[i].offset);

		uint8 key = 35;
		char c = 0;
		do {
			c = crypted->readByte() ^ key++;
			_phrases[i].string += c;
		} while (c);
	}

	delete crypted;

	// Create a surface to draw the subtitles on
	_surface = new Graphics::Surface();
	_surface->create(Renderer::kOriginalWidth, Renderer::kBottomBorderHeight, Graphics::PixelFormat(4, 8, 8, 8, 8, 0, 8, 16, 24));

	_texture = _vm->_gfx->createTexture(_surface);
}

Subtitles::~Subtitles() {
	if (_surface) {
		_surface->free();
		delete _surface;
	}
	if (_texture) {
		_vm->_gfx->freeTexture(_texture);
	}
}

void Subtitles::setFrame(int32 frame) {
	const Phrase *phrase = 0;

	for (uint i = 0; i < _phrases.size(); i++) {
		if (_phrases[i].frame > frame)
			break;

		phrase = &_phrases[i];
	}

	if (phrase == 0
			|| phrase->frame == _frame)
		return;

	_frame = phrase->frame;


	// TODO: Use the TTF font provided by the game
	// TODO: Use the positioning information provided by the game
	const Graphics::Font *font = FontMan.getFontByUsage(Graphics::FontManager::kLocalizedFont);

	if (!font)
		error("No available font");

	// Draw the new text
	memset(_surface->pixels, 0, _surface->pitch * _surface->h);
	font->drawString(_surface, phrase->string, 0, _surface->h / 2, _surface->w, 0xFFFFFFFF, Graphics::kTextAlignCenter);

	// Update the texture
	_texture->update(_surface);
}

void Subtitles::drawOverlay() {
	Common::Rect textureRect = Common::Rect(_texture->width, _texture->height);
	Common::Rect bottomBorder = textureRect;
	bottomBorder.translate(0, Renderer::kTopBorderHeight + Renderer::kFrameHeight);

	_vm->_gfx->drawTexturedRect2D(bottomBorder, textureRect, _texture);
}

} /* namespace Myst3 */