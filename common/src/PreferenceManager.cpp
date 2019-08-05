/*
 Copyright (C) 2010-2017 Kristian Duske

 This file is part of TrenchBroom.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#include "PreferenceManager.h"

namespace TrenchBroom {
    // PreferenceSerializerV1

    bool PreferenceSerializerV1::readFromString(const QString& in, bool* out) {
        if (in == QStringLiteral("1")) {
            *out = true;
            return true;
        } else if (in == QStringLiteral("0")) {
            *out = false;
            return true;
        } else {
            return false;
        }
    }

    bool PreferenceSerializerV1::readFromString(const QString& in, Color* out) {
        const std::string inStdString = in.toStdString();

        if (!Color::canParse(inStdString)) {
            return false;
        }
        
        *out = Color::parse(inStdString);
        return true;
    }

    bool PreferenceSerializerV1::readFromString(const QString& in, float* out) {
        auto inCopy = QString(in);
        auto inStream = QTextStream(&inCopy);

        inStream >> *out;

        return (inStream.status() == QTextStream::Ok);
    }

    bool PreferenceSerializerV1::readFromString(const QString& in, int* out) {
        auto inCopy = QString(in);
        auto inStream = QTextStream(&inCopy);

        inStream >> *out;

        return (inStream.status() == QTextStream::Ok);
    }

    bool PreferenceSerializerV1::readFromString(const QString& in, IO::Path* out) {
        *out = IO::Path::fromQString(in);
        return true;
    }

    bool PreferenceSerializerV1::readFromString(const QString& in, View::KeyboardShortcut* out) {
        nonstd::optional<View::KeyboardShortcut> result = 
            View::KeyboardShortcut::fromV1Settings(in);
        
        if (!result.has_value()) {
            return false;
        }
        *out = result.value();
        return true;
    }

    void PreferenceSerializerV1::writeToString(QTextStream& stream, const bool in) {
        if (in) {
            stream << "1";
        } else {
            stream << "0";
        }
    }

    void PreferenceSerializerV1::writeToString(QTextStream& stream, const Color& in) {
        // NOTE: QTextStream's default locale is C, unlike QString::arg()
        stream << in.r() << " "
               << in.g() << " "
               << in.b() << " "
               << in.a();
    }

    void PreferenceSerializerV1::writeToString(QTextStream& stream, const float in) {
        stream << in;
    }

    void PreferenceSerializerV1::writeToString(QTextStream& stream, const int in) {
        stream << in;
    }

    void PreferenceSerializerV1::writeToString(QTextStream& stream, const IO::Path& in) {
        // NOTE: this serializes with "\" separators on Windows and "/" elsewhere!
        stream << in.asQString();
    }

    void PreferenceSerializerV1::writeToString(QTextStream& stream, const View::KeyboardShortcut& in) {
        stream << in.toV1Settings();
    }

    // PreferenceManager

    void PreferenceManager::markAsUnsaved(PreferenceBase* preference) {
        m_unsavedPreferences.insert(preference);
    }

    PreferenceManager& PreferenceManager::instance() {
        static PreferenceManager prefs;
        return prefs;
    }

    bool PreferenceManager::saveInstantly() const {
        return m_saveInstantly;
    }

    PreferenceBase::Set PreferenceManager::saveChanges() {
        PreferenceBase::Set changedPreferences;
        for (auto* pref : m_unsavedPreferences) {
            pref->save();
            preferenceDidChangeNotifier(pref->path());

            changedPreferences.insert(pref);
        }

        m_unsavedPreferences.clear();
        return changedPreferences;
    }

    PreferenceBase::Set PreferenceManager::discardChanges() {
        PreferenceBase::Set changedPreferences;
        for (auto* pref : m_unsavedPreferences) {
            pref->resetToPrevious();
            changedPreferences.insert(pref);
        }

        m_unsavedPreferences.clear();
        return changedPreferences;
    }

    PreferenceManager::PreferenceManager() {
#if defined __APPLE__
        m_saveInstantly = true;
#else
        m_saveInstantly = false;
#endif
    }

    std::map<QString, std::map<QString, QString>> parseINI(QTextStream* iniStream) {
        QString section;
        std::map<QString, std::map<QString, QString>> result;

        while (!iniStream->atEnd()) {
            QString line = iniStream->readLine();

            // Trim leading/trailing whitespace
            line = line.trimmed();

            // Unescape escape sequences
            line.replace("\\ ", " ");

            // TODO: Handle comments, if we want to.

            const bool sqBracketAtStart = line.startsWith('[');
            const bool sqBracketAtEnd = line.endsWith(']');

            const bool heading = sqBracketAtStart && sqBracketAtEnd;
            if (heading) {
                section = line.mid(1, line.length() - 2);
                continue;
            }

            //  Not a heading, see if it's a key=value entry
            const int eqIndex = line.indexOf('=');
            if (eqIndex != -1) {
                QString key = line.left(eqIndex);
                QString value = line.mid(eqIndex + 1);

                result[section][key] = value;
                continue;
            }

            // Line was ignored
        }
        return result;
    }
}
