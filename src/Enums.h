// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2020 Yury Gubich <blue@macaw.me>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ENUMS_H
#define ENUMS_H

#include <QtGlobal>
#include <QObject>
#include <QMetaEnum>
#include <QXmppClient.h>

#define ENABLE_IF(...) typename std::enable_if<__VA_ARGS__>::type* = nullptr

template <typename... Ts> struct make_void { typedef void type; };
template <typename... Ts> using void_t = typename make_void<Ts...>::type;

// primary template handles types that have no nested ::enum_type member, like standard enum
template <typename, typename = void_t<>>
struct has_enum_type : std::false_type { };

// specialization recognizes types that do have a nested ::enum_type member, like QFlags enum
template <typename T>
struct has_enum_type<T, void_t<typename T::enum_type>> : std::true_type { };

namespace Enums {
	Q_NAMESPACE

	/**
	 * Enumeration of possible connection states.
	 */
	enum class ConnectionState {
		StateDisconnected = QXmppClient::DisconnectedState,
		StateConnecting = QXmppClient::ConnectingState,
		StateConnected = QXmppClient::ConnectedState
	};
	Q_ENUM_NS(ConnectionState)

	/**
	 * Enumeration of different media/message types
	 */
	enum class MessageType {
		MessageUnknown = -1,
		MessageText,
		MessageFile,
		MessageImage,
		MessageVideo,
		MessageAudio,
		MessageDocument,
		MessageGeoLocation
	};
	Q_ENUM_NS(MessageType)

	/**
	 * Enumeration of different message delivery states
	 */
	enum class DeliveryState {
		Pending,
		Sent,
		Delivered,
		Error,
		Draft
	};
	Q_ENUM_NS(DeliveryState)

	/**
	 * State which specifies how the XMPP login URI was used
	 */
	enum class LoginByUriState {
		Connecting,         ///< The JID and password are included in the URI and the client is connecting.
		PasswordNeeded,     ///< The JID is included in the URI but not the password.
		InvalidLoginUri     ///< The URI cannot be used to log in.
	};
	Q_ENUM_NS(LoginByUriState)

	template <typename T, ENABLE_IF(!has_enum_type<T>::value && std::is_enum<T>::value)>
	QString toString(const T flag) {
		static const QMetaEnum e = QMetaEnum::fromType<T>();
		return QString::fromLatin1(e.valueToKey(static_cast<int>(flag)));
	}

	template <typename T, ENABLE_IF(has_enum_type<T>::value)>
	QString toString(const T flags) {
		static const QMetaEnum e = QMetaEnum::fromType<T>();
		return QString::fromLatin1(e.valueToKeys(static_cast<int>(flags)));
	}
}

// Needed workaround to trigger older CMake auto moc versions to generate moc
// sources for this file (it only contains Q_NAMESPACE, which is new).
#if 0
Q_OBJECT
#endif

#endif // ENUMS_H
