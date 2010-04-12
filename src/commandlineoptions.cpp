/* This file is part of Clementine.

   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "commandlineoptions.h"

#include <cstdlib>
#include <getopt.h>
#include <iostream>

#include <QFileInfo>
#include <QBuffer>

const char* CommandlineOptions::kHelpText =
    "%1: clementine [%2] [%3]\n"
    "\n"
    "%4:\n"
    "  -p, --play                %5\n"
    "  -t, --play-pause          %6\n"
    "  -u, --pause               %7\n"
    "  -s, --stop                %8\n"
    "  -r, --previous            %9\n"
    "  -f, --next                %10\n"
    "  -v, --volume <value>      %11\n"
    "  --volume-up               %12\n"
    "  --volume-down             %13\n"
    "  --seek-to <seconds>       %14\n"
    "\n"
    "%15:\n"
    "  -a, --append              %16\n"
    "  -l, --load                %17\n"
    "  -k, --play-track <n>      %18\n"
    "\n"
    "%19:\n"
    "  -o, --show-osd            %20\n";


CommandlineOptions::CommandlineOptions(int argc, char** argv)
  : argc_(argc),
    argv_(argv),
    url_list_action_(UrlList_Append),
    player_action_(Player_None),
    set_volume_(-1),
    volume_modifier_(0),
    seek_to_(-1),
    play_track_at_(-1),
    show_osd_(false)
{
}

bool CommandlineOptions::Parse() {
  static const struct option kOptions[] = {
    {"help",       no_argument, 0, 'h'},

    {"play",       no_argument, 0, 'p'},
    {"play-pause", no_argument, 0, 't'},
    {"pause",      no_argument, 0, 'u'},
    {"stop",       no_argument, 0, 's'},
    {"previous",   no_argument, 0, 'r'},
    {"next",       no_argument, 0, 'f'},
    {"volume",     required_argument, 0, 'v'},
    {"volume-up",  no_argument, 0, VolumeUp},
    {"volume-down", no_argument, 0, VolumeDown},
    {"seek-to",    required_argument, 0, SeekTo},

    {"append",     no_argument, 0, 'a'},
    {"load",       no_argument, 0, 'l'},
    {"play-track", required_argument, 0, 'k'},

    {"show-osd",   no_argument, 0, 'o'},

    {0, 0, 0, 0}
  };

  // Parse the arguments
  int option_index = 0;
  bool ok = false;
  forever {
    int c = getopt_long(argc_, argv_, "hptusrfv:alk:o", kOptions, &option_index);

    // End of the options
    if (c == -1)
      break;

    switch (c) {
      case 'h': {
        QString translated_help_text = QString(kHelpText).arg(
            tr("Usage"), tr("options"), tr("URL(s)"), tr("Player options"),
            tr("Start the playlist currently playing"),
            tr("Play if stopped, pause if playing"),
            tr("Pause playback"),
            tr("Stop playback"),
            tr("Skip backwards in playlist")).arg(
            tr("Skip forwards in playlist"),
            tr("Set the volume to <value> percent"),
            tr("Increase the volume by 4%"),
            tr("Decrease the volume by 4%"),
            tr("Seek the currently playing track"),
            tr("Playlist options"),
            tr("Append files/URLs to the playlist"),
            tr("Loads files/URLs, replacing current playlist"),
            tr("Play the <n>th track in the playlist")).arg(
            tr("Other options"),
            tr("Display the on-screen-display"));

        std::cout << translated_help_text.toLocal8Bit().constData();
        return false;
      }

      case 'p': player_action_   = Player_Play;      break;
      case 't': player_action_   = Player_PlayPause; break;
      case 'u': player_action_   = Player_Pause;     break;
      case 's': player_action_   = Player_Stop;      break;
      case 'r': player_action_   = Player_Previous;  break;
      case 'f': player_action_   = Player_Next;      break;
      case 'a': url_list_action_ = UrlList_Append;   break;
      case 'l': url_list_action_ = UrlList_Load;     break;
      case 'o': show_osd_        = true;             break;
      case VolumeUp:   volume_modifier_ = +4;        break;
      case VolumeDown: volume_modifier_ = -4;        break;

      case 'v':
        set_volume_ = QString(optarg).toInt(&ok);
        if (!ok) set_volume_ = -1;
        break;

      case SeekTo:
        seek_to_ = QString(optarg).toInt(&ok);
        if (!ok) seek_to_ = -1;
        break;

      case 'k':
        play_track_at_ = QString(optarg).toInt(&ok);
        if (!ok) play_track_at_ = -1;
        break;

      case '?':
      default:
        return false;
    }
  }

  // Get any filenames or URLs following the arguments
  for (int i=option_index+1 ; i<argc_ ; ++i) {
    QString value = QString::fromLocal8Bit(argv_[i]);
    if (value.contains("://"))
      urls_ << value;
    else
      urls_ << QUrl::fromLocalFile(QFileInfo(value).absoluteFilePath());
  }

  return true;
}

bool CommandlineOptions::is_empty() const {
  return player_action_ == Player_None &&
         set_volume_ == -1 &&
         volume_modifier_ == 0 &&
         seek_to_ == -1 &&
         play_track_at_ == -1 &&
         show_osd_ == false &&
         urls_.isEmpty();
}

QByteArray CommandlineOptions::Serialize() const {
  QBuffer buf;
  buf.open(QIODevice::WriteOnly);

  QDataStream s(&buf);
  s << *this;
  buf.close();

  return buf.data();
}

void CommandlineOptions::Load(const QByteArray &serialized) {
  QByteArray copy(serialized);
  QBuffer buf(&copy);
  buf.open(QIODevice::ReadOnly);

  QDataStream s(&buf);
  s >> *this;
}

QString CommandlineOptions::tr(const char *source_text) {
  return QObject::tr(source_text);
}

QDataStream& operator<<(QDataStream& s, const CommandlineOptions& a) {
  s << qint32(a.player_action_)
    << qint32(a.url_list_action_)
    << a.set_volume_
    << a.volume_modifier_
    << a.seek_to_
    << a.play_track_at_
    << a.show_osd_
    << a.urls_;

  return s;
}

QDataStream& operator>>(QDataStream& s, CommandlineOptions& a) {
  s >> reinterpret_cast<qint32&>(a.player_action_)
    >> reinterpret_cast<qint32&>(a.url_list_action_)
    >> a.set_volume_
    >> a.volume_modifier_
    >> a.seek_to_
    >> a.play_track_at_
    >> a.show_osd_
    >> a.urls_;

  return s;
}
