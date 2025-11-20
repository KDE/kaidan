// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QSortFilterProxyModel>
// Kaidan
#include "Globals.h"

class ProviderFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(bool supportsInBandRegistrationOnly MEMBER m_supportsInBandRegistrationOnly WRITE setSupportsInBandRegistrationOnly NOTIFY
                   supportsInBandRegistrationOnlyChanged)
    Q_PROPERTY(QString flag MEMBER m_flag WRITE setFlag NOTIFY flagChanged)
    Q_PROPERTY(int mininumBusFactor MEMBER m_mininumBusFactor WRITE setMininumBusFactor NOTIFY mininumBusFactorChanged)
    Q_PROPERTY(QString organization MEMBER m_organization WRITE setOrganization NOTIFY organizationChanged)
    Q_PROPERTY(bool supportsPasswordResetOnly MEMBER m_supportsPasswordResetOnly WRITE setSupportsPasswordResetOnly NOTIFY supportsPasswordResetOnlyChanged)
    Q_PROPERTY(bool hostedGreenOnly MEMBER m_hostedGreenOnly WRITE setHostedGreenOnly NOTIFY hostedGreenOnlyChanged)
    Q_PROPERTY(QList<int> availabilities READ availabilities CONSTANT)
    Q_PROPERTY(int minimumAvailability MEMBER m_minimumAvailability WRITE setMinimumAvailability NOTIFY minimumAvailabilityChanged)
    Q_PROPERTY(QList<int> httpUploadFileSizes READ httpUploadFileSizes CONSTANT)
    Q_PROPERTY(int minimumHttpUploadFileSize MEMBER m_minimumHttpUploadFileSize WRITE setMinimumHttpUploadFileSize NOTIFY minimumHttpUploadFileSizeChanged)
    Q_PROPERTY(QList<int> httpUploadTotalSizes READ httpUploadTotalSizes CONSTANT)
    Q_PROPERTY(int minimumHttpUploadTotalSize MEMBER m_minimumHttpUploadTotalSize WRITE setMinimumHttpUploadTotalSize NOTIFY minimumHttpUploadTotalSizeChanged)
    Q_PROPERTY(QList<int> httpUploadStorageDurations READ httpUploadStorageDurations CONSTANT)
    Q_PROPERTY(int minimumHttpUploadStorageDuration MEMBER m_minimumHttpUploadStorageDuration WRITE setMinimumHttpUploadStorageDuration NOTIFY
                   minimumHttpUploadStorageDurationChanged)
    Q_PROPERTY(QList<int> messageStorageDurations READ messageStorageDurations CONSTANT)
    Q_PROPERTY(int minimumMessageStorageDuration MEMBER m_minimumMessageStorageDuration WRITE setMinimumMessageStorageDuration NOTIFY
                   minimumMessageStorageDurationChanged)
    Q_PROPERTY(bool freeOfChargeOnly MEMBER m_freeOfChargeOnly WRITE setFreeOfChargeOnly NOTIFY freeOfChargeOnlyChanged)

public:
    explicit ProviderFilterModel(QObject *parent = nullptr);

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    void setSupportsInBandRegistrationOnly(bool supportsInBandRegistrationOnly);
    Q_SIGNAL void supportsInBandRegistrationOnlyChanged();

    void setFlag(const QString &flag);
    Q_SIGNAL void flagChanged();

    void setMininumBusFactor(int mininumBusFactor);
    Q_SIGNAL void mininumBusFactorChanged();

    void setOrganization(const QString &organization);
    Q_SIGNAL void organizationChanged();

    void setSupportsPasswordResetOnly(bool supportsPasswordResetOnly);
    Q_SIGNAL void supportsPasswordResetOnlyChanged();

    void setHostedGreenOnly(bool hostedGreenOnly);
    Q_SIGNAL void hostedGreenOnlyChanged();

    static QList<int> availabilities();

    void setMinimumAvailability(int minimumAvailability);
    Q_SIGNAL void minimumAvailabilityChanged();

    static QList<int> httpUploadFileSizes();

    void setMinimumHttpUploadFileSize(int minimumHttpUploadFileSize);
    Q_SIGNAL void minimumHttpUploadFileSizeChanged();

    static QList<int> httpUploadTotalSizes();

    void setMinimumHttpUploadTotalSize(int minimumHttpUploadTotalSize);
    Q_SIGNAL void minimumHttpUploadTotalSizeChanged();

    static QList<int> httpUploadStorageDurations();

    void setMinimumHttpUploadStorageDuration(int minimumHttpUploadStorageDuration);
    Q_SIGNAL void minimumHttpUploadStorageDurationChanged();

    static QList<int> messageStorageDurations();

    void setMinimumMessageStorageDuration(int minimumMessageStorageDuration);
    Q_SIGNAL void minimumMessageStorageDurationChanged();

    void setFreeOfChargeOnly(bool freeOfChargeOnly);
    Q_SIGNAL void freeOfChargeOnlyChanged();

private:
    bool m_supportsInBandRegistrationOnly = false;
    QString m_flag = NO_SELECTION_TEXT;
    int m_mininumBusFactor = 1;
    QString m_organization = NO_SELECTION_TEXT;
    bool m_supportsPasswordResetOnly = false;
    bool m_hostedGreenOnly = false;
    int m_minimumAvailability = -1;
    int m_minimumHttpUploadFileSize = -1;
    int m_minimumHttpUploadTotalSize = -1;
    int m_minimumHttpUploadStorageDuration = -1;
    int m_minimumMessageStorageDuration = -1;
    bool m_freeOfChargeOnly = false;
};
