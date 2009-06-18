/* Residual - A 3D game interpreter
 *
 * Residual is the legal property of its developers, whose names
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
 * $URL$
 * $Id$
 *
 */

#ifndef COMMON_ARCHIVE_H
#define COMMON_ARCHIVE_H

#include "common/str.h"
#include "common/hash-str.h"
#include "common/list.h"
#include "common/ptr.h"
#include "common/singleton.h"

namespace Common {

class FSNode;
class SeekableReadStream;


/**
 * ArchiveMember is an abstract interface to represent elements inside
 * implementations of Archive.
 *
 * Archive subclasses must provide their own implementation of ArchiveMember,
 * and use it when serving calls to listMembers() and listMatchingMembers().
 * Alternatively, the GenericArchiveMember below can be used.
 */
class ArchiveMember {
public:
	virtual ~ArchiveMember() { }
	virtual SeekableReadStream *createReadStream() const = 0;
	virtual String getName() const = 0;
	virtual String getDisplayName() const { return getName(); }
};

typedef SharedPtr<ArchiveMember> ArchiveMemberPtr;
typedef List<ArchiveMemberPtr> ArchiveMemberList;

class Archive;

/**
 * Simple ArchiveMember implementation which allows
 * creation of ArchiveMember compatible objects via
 * a simple Archive and name pair.
 *
 * Note that GenericArchiveMember objects will not
 * be working anymore after the 'parent' object
 * is destroyed.
 */
class GenericArchiveMember : public ArchiveMember {
	Archive *_parent;
	String _name;
public:
	GenericArchiveMember(String name, Archive *parent);
	String getName() const;
	SeekableReadStream *createReadStream() const;
};


/**
 * Archive allows searches of (file)names into an arbitrary container.
 * It also supports opening a file and returning an usable input stream.
 */
class Archive {
public:
	virtual ~Archive() { }

	/**
	 * Check if a name is present in the Archive. Patterns are not allowed,
	 * as this is meant to be a quick File::exists() replacement.
	 */
	virtual bool hasFile(const String &name) = 0;

	/**
	 * Add all the names present in the Archive which match pattern to
	 * list. Returned names can be used as parameters to createReadStreamForMember.
	 * Must not remove elements from the list.
	 *
	 * @return the number of names added to list
	 */
	virtual int listMatchingMembers(ArchiveMemberList &list, const String &pattern);

	/**
	 * Add all the names present in the Archive to list. Returned
	 * names can be used as parameters to createReadStreamForMember.
	 * Must not remove elements from the list.
	 *
	 * @return the number of names added to list
	 */
	virtual int listMembers(ArchiveMemberList &list) = 0;

	/**
	 * Returns a ArchiveMember representation of the given file.
	 */
	virtual ArchiveMemberPtr getMember(const String &name) = 0;

	/**
	 * Create a stream bound to a member in the archive. If no member with the
	 * specified name exists, then 0 is returned.
	 * @return the newly created input stream
	 */
	virtual SeekableReadStream *createReadStreamForMember(const String &name) const = 0;
};


/**
 * SearchSet enables access to a group of Archives through the Archive interface.
 * Its intended usage is a situation in which there are no name clashes among names in the
 * contained Archives, hence the simplistic policy of always looking for the first
 * match. SearchSet *DOES* guarantee that searches are performed in *DESCENDING*
 * priority order. In case of conflicting priorities, insertion order prevails.
 */
class SearchSet : public Archive {
	struct Node {
		int		_priority;
		String	_name;
		Archive	*_arc;
		bool	_autoFree;
		Node(int priority, const String &name, Archive *arc, bool autoFree)
			: _priority(priority), _name(name), _arc(arc), _autoFree(autoFree) {
		}
	};
	typedef List<Node> ArchiveNodeList;
	ArchiveNodeList _list;

	ArchiveNodeList::iterator find(const String &name);
	ArchiveNodeList::const_iterator find(const String &name) const;

	// Add an archive keeping the list sorted by ascending priorities.
	void insert(const Node& node);

public:
	virtual ~SearchSet() { clear(); }

	/**
	 * Add a new archive to the searchable set.
	 */
	void add(const String& name, Archive *arch, int priority = 0, bool autoFree = true);

	/**
	 * Create and add a FSDirectory by name
	 */
	void addDirectory(const String &name, const String &directory, int priority = 0, int depth = 1, bool flat = false);

	/**
	 * Create and add a FSDirectory by FSNode
	 */
	void addDirectory(const String &name, const FSNode &directory, int priority = 0, int depth = 1, bool flat = false);

	/**
	 * Remove an archive from the searchable set.
	 */
	void remove(const String& name);

	/**
	 * Check if a given archive name is already present.
	 */
	bool hasArchive(const String &name) const;

	/**
	 * Empties the searchable set.
	 */
	virtual void clear();

	/**
	 * Change the order of searches.
	 */
	void setPriority(const String& name, int priority);

	virtual bool hasFile(const String &name);
	virtual int listMatchingMembers(ArchiveMemberList &list, const String &pattern);
	virtual int listMembers(ArchiveMemberList &list);

	virtual ArchiveMemberPtr getMember(const String &name);

	/**
	 * Implements createReadStreamForMember from Archive base class. The current policy is
	 * opening the first file encountered that matches the name.
	 */
	virtual SeekableReadStream *createReadStreamForMember(const String &name) const;
};


class SearchManager : public Singleton<SearchManager>, public SearchSet {
public:

	/**
	 * Resets the search manager to the default list of search paths (system
	 * specific dirs + current dir).
	 */
	virtual void clear();

private:
	friend class Common::Singleton<SingletonBaseType>;
	SearchManager();
};

/** Shortcut for accessing the search manager. */
#define SearchMan		Common::SearchManager::instance()

} // namespace Common

#endif
