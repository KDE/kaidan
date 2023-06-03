#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2018 Linus Jahn <lnj@kaidan.im>
#
# SPDX-License-Identifier: MIT

"""
This script generates a debian-formatted, machine-readable copyright file for
Kaidan. It uses git to get a list of contributors for the source code and the
translations.
"""

import git
import datetime

# These user ids will be replaced for the LICENSE file
# If you want to be added to this list, please open an issue or pull request!
REPLACE_USER_IDS = [
	("Ellenjott [LNJ] <git@lnj.li>", "Linus Jahn <lnj@kaidan.im>"),
	("LNJ <git@lnj.li>", "Linus Jahn <lnj@kaidan.im>"),
	("Linus Jahn <linus.jahn@searchmetrics.com>", "Linus Jahn <lnj@kaidan.im>"),
	("JBBgameich <jbbgameich@outlook.com>", "Jonah Brüchert <jbb@kaidan.im>"),
	("JBBgameich <jbbgameich@gmail.com>", "Jonah Brüchert <jbb@kaidan.im>"),
	("JBBgameich <jbb.prv@gmx.de>", "Jonah Brüchert <jbb@kaidan.im>"),
	("JBB <jbb.prv@gmx.de>", "Jonah Brüchert <jbb@kaidan.im>"),
	("Jonah Brüchert <jbb.prv@gmx.de>", "Jonah Brüchert <jbb@kaidan.im>"),
	("Jonah Brüchert <jbb.mail@gmx.de>", "Jonah Brüchert <jbb@kaidan.im>"),
	("Jonah Bruechert <jbb.mail@gmx.de>", "Jonah Brüchert <jbb@kaidan.im>"),
	("Georg <s.g.b@gmx.de>", "geobra <s.g.b@gmx.de>"),
	("Muhammad Nur Hidayat Yasuyoshi (MNH48.com) <muhdnurhidayat96@yahoo.com>", "Muhammad Nur Hidayat Yasuyoshi <translation@mnh48.moe>"),
	("Muhammad Nur Hidayat Yasuyoshi <mnh48mail@gmail.com>", "Muhammad Nur Hidayat Yasuyoshi <translation@mnh48.moe>"),
	("Muhammad Nur Hidayat Yasuyoshi <admin@mnh48.moe>", "Muhammad Nur Hidayat Yasuyoshi <translation@mnh48.moe>"),
	("Yaya - Nurul Azeera Hidayah @ Muhammad Nur Hidayat Yasuyoshi <translation@mnh48.moe>", "Muhammad Nur Hidayat Yasuyoshi <translation@mnh48.moe>"),
	("Joeke de Graaf <mappack@null.net>", "Joeke de Graaf <joeke.de.graaf@student.nhlstenden.com>"),
	("X advocatux <advocatux@airpost.net>", "advocatux <advocatux@airpost.net>"),
	("ZatroxDE <zatroxde@outlook.com>", "Robert Maerkisch <zatroxde@protonmail.ch>"),
	("X oiseauroch <tobias.ollive@mailoo.org>", "oiseauroch <tobias.ollive@mailoo.org>"),
	("X ssantos <ssantos@web.de>", "ssantos <ssantos@web.de>"),
	("ssantos _ <ssantos@web.de>", "ssantos <ssantos@web.de>"),
	("X Txaume <txaumevw@gmail.com>", "Txaume <txaumevw@gmail.com>"),
	("X mondstern <hello@mondstern.tk>", "mondstern <mondstern@snopyta.org>"),
	("X mondstern <mondstern@snopyta.org>", "mondstern <mondstern@snopyta.org>"),
	("Jeannette L <j.lavoie@net-c.ca>", "Jeannette Lavoie <j.lavoie@net-c.ca>"),
	("aevw _ <arcanevw@tuta.io>", "aevw <arcanevw@tuta.io>"),
    ("Coucouf _ <coucouf@coucouf.fr>", "Coucouf <coucouf@coucouf.fr>"),
	("t4llkr _ <anaterem2015@yandex.ru>", "t4llkr <anaterem2015@yandex.ru>"),
	("papadop _ <papadop@protonmail.com>", "papadop <papadop@protonmail.com>"),
	("Anna _ <bs@sysrq.in>", "Anna <bs@sysrq.in>"),
	("marek _ <Marek@walczak.io>", "marek <marek@walczak.io>"),
    ("TigranKhachatryan0 _ <tigrank2008@gmail.com>", "TigranKhachatryan0 <tigrank2008@gmail.com>"),
]

# These user ids will be excluded from any targets
EXCLUDE_USER_IDS = [
	"Weblate <noreply@weblate.org>",
	"Hosted Weblate <hosted@weblate.org>",
	"anonymous <>",
	"anonymous <> <None>",
	"Kaidan Translations <translations@kaidan.im>",
	"Kaidan translations <translations@kaidan.im>",
	"Hosted Weblate <hosted@weblate.org>",
	"l10n daemon script <scripty@kde.org>",
	"Anonymous <noreply@weblate.org>",
	"Anonymous _ <noreply@weblate.org>",
	"X anonymous <noreply@weblate.org>"
]

GPL3_OPENSSL_LICENSE = """This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 3 of the License, or (at your option) any later
version.

In addition, as a special exception, the author of this program gives permission
to link the code of its release with the OpenSSL project's "OpenSSL" library (or
with modified versions of it that use the same license as the "OpenSSL"
library), and distribute the linked executables. You must obey the GNU General
Public License in all respects for all of the code used other than "OpenSSL".
If you modify this file, you may extend this exception to your version of the
file, but you are not obligated to do so.  If you do not wish to do so, delete
this exception statement from your version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this package.  If not, see <http://www.gnu.org/licenses/>.

On Debian systems, the full text of the GNU General Public License version 3 can
be found in the file
`/usr/share/common-licenses/GPL-3'."""

GPL3_LICENSE = """This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this package; if not, write to the Free Software Foundation, Inc., 51 Franklin
St, Fifth Floor, Boston, MA  02110-1301 USA

On Debian systems, the full text of the GNU General Public
License version 3 can be found in the file
`/usr/share/common-licenses/GPL-3'."""

MIT_LICENSE = """Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE."""

MIT_APPLE_LICENSE = """Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE."""

APACHE2_LICENSE = """Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License."""

class CopyrightAuthor:
	def __init__(self, score = 0, dates = None, years = "", uid = ""):
		self.dates = dates or list([]);
		self.score = score;
		self.years = years;
		self.uid = uid;

	def addTimestamp(self, x):
		self.dates.append(x);

def createField(name, contentList, fullIndent = True):
	retvar = "{}: {}\n".format(name, contentList[0]);

	if len(contentList) < 2:
		return retvar;

	indent = len(name) * " ";
	for i in range(1, len(contentList)):
		retvar += "{}  {}\n".format(indent, contentList[i]);

	return retvar;

def createLongField(name, heading, content):
	retvar = "{}: {}\n".format(name, heading);

	if not content:
		return retvar;

	for line in content.split('\n'):
		if not line.strip(): # if line is empty
			line = ".";
		retvar += " {}\n".format(line);
	return retvar;

class CopyrightTarget:
	def __init__(self, directories = None, files = None, licenseName = "",
		     licenseContent = "", replaceUids = None, excludeUids = None,
		     authorList = None, additionalAuthors = None, comment = ""):
		self.repo = git.Repo(".");
		self.files = files or list([]);
		self.directories = directories or list([]);
		self.licenseName = licenseName;
		self.licenseContent = licenseContent
		self.authorList = authorList or dict({});
		self.comment = comment
		self.replaceUids = replaceUids or list([]);
		self.excludeUids = excludeUids or list([]);
		self.replaceUids.extend(REPLACE_USER_IDS);
		self.excludeUids.extend(EXCLUDE_USER_IDS);

		self.authorList = authorList or self.getAuthorList();
		if additionalAuthors:
			self.authorList.update(additionalAuthors)

	def replaceUid(self, uid):
		for pair in self.replaceUids:
			if uid == pair[0]:
				uid = pair[1];
		return uid;

	def getAuthorList(self):
		paths = list(self.files);
		paths.extend(self.directories);

		commitList = self.repo.iter_commits(paths=paths);
		authorList = {};

		for commit in commitList:
			# create user id and check replacements and excludes
			uid = "{} <{}>".format(commit.author.name,
					       commit.author.email);
			uid = self.replaceUid(uid);

			if uid in self.excludeUids:
				continue;

			if not uid in authorList.keys():
				authorList[uid] = CopyrightAuthor(uid = uid);

			authorList[uid].addTimestamp(commit.authored_date);

		for uid in authorList:
			minT = min(int(t) for t in authorList[uid].dates);
			maxT = max(int(t) for t in authorList[uid].dates);
			authorList[uid].score = maxT - minT;

			minYear = datetime.datetime.fromtimestamp(minT).year;
			maxYear = datetime.datetime.fromtimestamp(maxT).year;
			if minYear == maxYear:
				authorList[uid].years = str(minYear);
			else:
				authorList[uid].years = "{}-{}".format(minYear, maxYear);

			authorList[uid].dates = [];

		return authorList;

	def toDebianCopyright(self):
		# Create copyright list
		copyrights = [];
		for item in sorted(self.authorList.items(), key=lambda x: x[1].score, reverse=True):
			copyrights.append("{}, {}".format(item[1].years, item[0]));

		files = list(self.files);
		for directory in self.directories:
			files.append(directory + "/*");

		retvar = createField("Files", files);
		retvar += createField("Copyright", copyrights);
		retvar += createLongField("License", self.licenseName, self.licenseContent);
		if self.comment:
			retvar += createLongField("Comment", "", self.comment);

		return retvar;

class LicenseTarget:
	def __init__(self, name = "", content = ""):
		self.name = name;
		self.content = content;

	def toDebianCopyright(self):
		return createLongField("License", self.name, self.content);

def main():
	copyrightTargets = [
		CopyrightTarget(
			directories = ["src", "utils", "misc"],
			licenseName = "GPL-3+ with OpenSSL exception",
			additionalAuthors = {
				"Eike Hein <hein@kde.org>": CopyrightAuthor(years = "2017-2018")
			}
		),
		CopyrightTarget(
			files = ["src/StatusBar.cpp", "src/StatusBar.h", "src/singleapp/*",
				 "src/hsluv-c/*", "utils/generate-license.py"],
			licenseName = "MIT",
			authorList = {
				"J-P Nurmi <jpnurmi@gmail.com>": CopyrightAuthor(years = "2016"),
				"Linus Jahn <lnj@kaidan.im>": CopyrightAuthor(years = "2018-2019"),
				"Itay Grudev <itay+github.com@grudev.com>": CopyrightAuthor(years = "2015-2018"),
				"Alexei Boronine <alexei@boronine.com>": CopyrightAuthor(years = "2015"),
				"Roger Tallada <info@rogertallada.com>": CopyrightAuthor(years = "2015"),
				"Martin Mitas <mity@morous.org>": CopyrightAuthor(years = "2017"),
			}
		),
		CopyrightTarget(
			files = ["src/EmojiModel.cpp", "src/EmojiModel.h", "qml/elements/EmojiPicker.qml"],
			licenseName = "GPL-3+",
			authorList = {
				"Konstantinos Sideris <siderisk@auth.gr>": CopyrightAuthor(years = "2017"),
			},
		),
		CopyrightTarget(
			files = ["src/QrCodeVideoFrame.h", "src/QrCodeVideoFrame.cpp"],
			licenseName = "apache-2.0",
			authorList = {
				"QZXing authors": CopyrightAuthor(years = "2017"),
			},
		),
		CopyrightTarget(
			files = ["data/images/check-mark.svg"],
			licenseName = "CC-BY-SA-4.0",
			authorList = {
				"Melvin Keskin <melvo@olomono.de>": CopyrightAuthor(years = "2019"),
			},
		),
		CopyrightTarget(
			files = ["data/images/check-mark-pale.svg"],
			licenseName = "CC-BY-SA-4.0",
			authorList = {
				"Melvin Keskin <melvo@olomono.de>": CopyrightAuthor(years = "2020"),
				"Yury Gubich <blue@macaw.me>": CopyrightAuthor(years = "2020"),
			},
		),
		CopyrightTarget(
			files = ["data/images/cross.svg", "data/images/dots.svg"],
			licenseName = "CC-BY-SA-4.0",
			authorList = {
				"Yury Gubich <blue@macaw.me>": CopyrightAuthor(years = "2020"),
			},
		),
		CopyrightTarget(
			files = [
				"misc/kaidan-128x128.png",
				"data/images/kaidan.svg", "data/images/kaidan-bw.svg"
			],
			licenseName = "CC-BY-SA-4.0",
			authorList = {
				"Ilya Bizyaev <bizyaev@zoho.com>": CopyrightAuthor(years = "2016-2017"),
				"Mathis Brüchert <mbblp@protonmail.ch>": CopyrightAuthor(years = "2020"),
				"Melvin Keskin <melvo@olomono.de>": CopyrightAuthor(years = "2020"),
			}
		),
		CopyrightTarget(
			files = ["data/images/chat-page-background.svg"],
			licenseName = "CC-BY-SA-4.0",
			authorList = {
				"Raghavendra Kamath <raghu@raghukamath.com>": CopyrightAuthor(years = "2022"),
			},
		),
		CopyrightTarget(
			files = [
				"data/images/onboarding/account-transfer.svg",
				"data/images/onboarding/automatic-registration.svg",
				"data/images/onboarding/custom-form.svg",
				"data/images/onboarding/display-name.svg",
				"data/images/onboarding/login.svg",
				"data/images/onboarding/manual-registration.svg",
				"data/images/onboarding/password.svg",
				"data/images/onboarding/registration.svg",
				"data/images/onboarding/result.svg",
				"data/images/onboarding/server.svg",
				"data/images/onboarding/username.svg",
				"data/images/onboarding/web-registration.svg"
			],
			licenseName = "CC-BY-SA-4.0",
			authorList = {
				"Mathis Brüchert <mbblp@protonmail.ch>": CopyrightAuthor(years = "2020")
			}
		),
		CopyrightTarget(
			files = ["data/images/mic0.svg", "data/images/mic1.svg", "data/images/mic2.svg", "data/images/mic3.svg"],
			licenseName = "CC-BY-SA-4.0",
			authorList = {
				"Jonah Brüchert <jbb@kaidan.im>": CopyrightAuthor(years = "2020"),
				"Mathis Brüchert <mbblp@protonmail.ch>": CopyrightAuthor(years = "2020")
			}
		),
		CopyrightTarget(
			files = [
				"data/images/qr-code-scan-1.svg",
				"data/images/qr-code-scan-2.svg",
			],
			licenseName = "CC-BY-SA-4.0",
			authorList = {
				"Mathis Brüchert <mbblp@protonmail.ch>": CopyrightAuthor(years = "2021")
			}
		),
		CopyrightTarget(
			files = [
				"data/images/qr-code-scan-own-1.svg",
				"data/images/qr-code-scan-own-2.svg",
			],
			licenseName = "CC-BY-SA-4.0",
			authorList = {
				"Mathis Brüchert <mbblp@protonmail.ch>": CopyrightAuthor(years = "2021"),
				"Melvin Keskin <melvo@olomono.de>": CopyrightAuthor(years = "2022")
			}
		),
		CopyrightTarget(
			files = ["utils/convert-prl-libs-to-cmake.pl"],
			licenseName = "MIT-Apple",
			authorList = {
				"Konstantin Tokarev <annulen@yandex.ru>": CopyrightAuthor(years = "2016")
			}
		),
		LicenseTarget(
			name = "GPL-3+ with OpenSSL exception",
			content = GPL3_OPENSSL_LICENSE
		),
		LicenseTarget(
			name = "GPL-3+",
			content = GPL3_LICENSE
		),
		LicenseTarget(
			name = "MIT",
			content = MIT_LICENSE
		),
		LicenseTarget(
			name = "MIT-Apple",
			content = MIT_APPLE_LICENSE
		),
		LicenseTarget(
			name = "apache-2.0",
			content = APACHE2_LICENSE
		)
	];

	print("Upstream-Name: kaidan")
	print("Source: https://invent.kde.org/network/kaidan/")
	print("Format: https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/")
	print()

	for target in copyrightTargets:
		print(target.toDebianCopyright());

if __name__ == "__main__":
	main();
