/* Residual - Virtual machine to run LucasArts' 3D adventure games
 *
 * Residual is the legal property of its developers, whose names
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
 * $URL$
 * $Id$
 *
 */

#if !defined(BACKEND_EVENTS_DEFAULT_H) && !defined(DISABLE_DEFAULT_EVENTMANAGER)
#define BACKEND_EVENTS_DEFAULT_H

#include "common/events.h"
#include "common/savefile.h"

#include "engine/backend/platform/driver.h"

/*
At some point we will remove pollEvent from OSystem and change
DefaultEventManager to use a "boss" derived from this class:
class EventProvider {
public
	virtual bool pollEvent(Common::Event &event) = 0;
};

Backends which wish to use the DefaultEventManager then simply can
use a subclass of EventProvider.
*/

class DefaultEventManager : public Common::EventManager {
	Driver *_boss;

	Common::Point _mousePos;
	int _buttonState;
	int _modifierState;
	bool _shouldQuit;

	class RandomSourceRecord {
	public:
		Common::String name;
		uint32 seed;
	};
	Common::Array<RandomSourceRecord> _randomSourceRecords;

	bool _recordSubtitles;
	volatile uint32 _recordCount;
	volatile uint32 _lastRecordEvent;
	volatile uint32 _recordTimeCount;
	Common::OutSaveFile *_recordFile;
	Common::OutSaveFile *_recordTimeFile;
	Common::MutexRef _timeMutex;
	Common::MutexRef _recorderMutex;
	volatile uint32 _lastMillis;

	volatile uint32 _playbackCount;
	volatile uint32 _playbackDiff;
	volatile bool _hasPlaybackEvent;
	volatile uint32 _playbackTimeCount;
	Common::Event _playbackEvent;
	Common::InSaveFile *_playbackFile;
	Common::InSaveFile *_playbackTimeFile;

	volatile uint32 _eventCount;
	volatile uint32 _lastEventCount;

	enum RecordMode {
		kPassthrough = 0,
		kRecorderRecord = 1,
		kRecorderPlayback = 2
	};
	volatile RecordMode _recordMode;
	Common::String _recordFileName;
	Common::String _recordTempFileName;
	Common::String _recordTimeFileName;

	// for continuous events (keyDown)
	enum {
		kKeyRepeatInitialDelay = 400,
		kKeyRepeatSustainDelay = 100
	};

	struct {
		uint16 ascii;
		byte flags;
		int keycode;
	} _currentKeyDown;
	uint32 _keyRepeatTime;

	void record(Common::Event &event);
	bool playback(Common::Event &event);
public:
	DefaultEventManager(Driver *boss);
	~DefaultEventManager();

	virtual bool pollEvent(Common::Event &event);
	virtual void registerRandomSource(Common::RandomSource &rnd, const char *name);
	virtual void processMillis(uint32 &millis);

	virtual Common::Point getMousePos() const { return _mousePos; }
	virtual int getButtonState() const { return _buttonState; }
	virtual int getModifierState() const { return _modifierState; }
	virtual int shouldQuit() const { return _shouldQuit; }
};

#endif
