// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ProviderFilterModel.h"

// Qt
#include <QDateTime>
// Kaidan
#include "ProviderModel.h"

const QList<int> AVAILABILITIES = {1, 3, 5, 10, 15};
const QList<int> HTTP_UPLOAD_FILE_SIZES = {20, 50, 100, 500, 1000};
const QList<int> HTTP_UPLOAD_TOTAL_SIZES = {100, 500, 1000, 3000, 5000};
const QList<int> HTTP_UPLOAD_STORAGE_DURATIONS = {7, 14, 30, 90, 180};
const QList<int> MESSAGE_STORAGE_DURATIONS = {7, 14, 30, 90, 180};

ProviderFilterModel::ProviderFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setMinimumAvailability(AVAILABILITIES.first());
    setMinimumHttpUploadFileSize(HTTP_UPLOAD_FILE_SIZES.first());
    setMinimumHttpUploadTotalSize(HTTP_UPLOAD_TOTAL_SIZES.first());
    setMinimumHttpUploadStorageDuration(HTTP_UPLOAD_STORAGE_DURATIONS.first());
    setMinimumMessageStorageDuration(MESSAGE_STORAGE_DURATIONS.first());
}

bool ProviderFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const auto model = static_cast<ProviderModel *>(sourceModel());
    QModelIndex index = model->index(sourceRow, 0, sourceParent);

    if (model->data(index, ProviderModel::Role::IsCustomProvider).toBool()) {
        return true;
    }

    const auto since = model->data(index, ProviderModel::Role::Since).toDate();
    const auto today = QDateTime::currentDateTimeUtc().date();
    int existence = 0;

    if (const auto years = today.year() - since.year()) {
        if (since.month() < today.month()) {
            existence = years;
        } else if (since.day() < today.day()) {
            existence = years;
        } else {
            existence = years - 1;
        }
    }

    const auto httpUploadFileSize = model->data(index, ProviderModel::Role::HttpUploadFileSize).toInt();
    const auto httpUploadTotalSize = model->data(index, ProviderModel::Role::HttpUploadTotalSize).toInt();
    const auto httpUploadStorageDuration = model->data(index, ProviderModel::Role::HttpUploadStorageDuration).toInt();
    const auto messageStorageDuration = model->data(index, ProviderModel::Role::MessageStorageDuration).toInt();

    return (!m_supportsInBandRegistrationOnly || model->data(index, ProviderModel::Role::SupportsInBandRegistration).toBool())
        && (m_flag == NO_SELECTION_TEXT || model->data(index, ProviderModel::Role::Flags).toList().contains(m_flag))
        && model->data(index, ProviderModel::Role::BusFactor).toInt() >= m_mininumBusFactor
        && (m_organization == NO_SELECTION_TEXT || model->data(index, ProviderModel::Role::Organization).toString() == m_organization)
        && (!m_supportsPasswordResetOnly || model->data(index, ProviderModel::Role::SupportsPasswordReset).toBool())
        && (!m_hostedGreenOnly || model->data(index, ProviderModel::Role::HostedGreen).toBool()) && (existence >= m_minimumAvailability)
        && (httpUploadFileSize == 0 || httpUploadFileSize >= m_minimumHttpUploadFileSize)
        && (httpUploadTotalSize == 0 || httpUploadTotalSize >= m_minimumHttpUploadTotalSize)
        && (httpUploadStorageDuration == 0 || httpUploadStorageDuration >= m_minimumHttpUploadStorageDuration)
        && (messageStorageDuration == 0 || messageStorageDuration >= m_minimumMessageStorageDuration)
        && (!m_freeOfChargeOnly || model->data(index, ProviderModel::Role::FreeOfCharge).toBool());
}

void ProviderFilterModel::setSupportsInBandRegistrationOnly(bool supportsInBandRegistrationOnly)
{
    if (m_supportsInBandRegistrationOnly != supportsInBandRegistrationOnly) {
        m_supportsInBandRegistrationOnly = supportsInBandRegistrationOnly;
        invalidate();
        Q_EMIT supportsInBandRegistrationOnlyChanged();
    }
}

void ProviderFilterModel::setFlag(const QString &flag)
{
    if (m_flag != flag) {
        m_flag = flag;
        invalidate();
        Q_EMIT flagChanged();
    }
}

void ProviderFilterModel::setMininumBusFactor(int mininumBusFactor)
{
    if (m_mininumBusFactor != mininumBusFactor) {
        m_mininumBusFactor = mininumBusFactor;
        invalidate();
        Q_EMIT mininumBusFactorChanged();
    }
}

void ProviderFilterModel::setOrganization(const QString &organization)
{
    if (m_organization != organization) {
        m_organization = organization;
        invalidate();
        Q_EMIT organizationChanged();
    }
}

void ProviderFilterModel::setSupportsPasswordResetOnly(bool supportsPasswordResetOnly)
{
    if (m_supportsPasswordResetOnly != supportsPasswordResetOnly) {
        m_supportsPasswordResetOnly = supportsPasswordResetOnly;
        invalidate();
        Q_EMIT supportsPasswordResetOnlyChanged();
    }
}

void ProviderFilterModel::setHostedGreenOnly(bool hostedGreenOnly)
{
    if (m_hostedGreenOnly != hostedGreenOnly) {
        m_hostedGreenOnly = hostedGreenOnly;
        invalidate();
        Q_EMIT hostedGreenOnlyChanged();
    }
}

QList<int> ProviderFilterModel::availabilities()
{
    return AVAILABILITIES;
}

void ProviderFilterModel::setMinimumAvailability(int minimumAvailability)
{
    if (m_minimumAvailability != minimumAvailability) {
        m_minimumAvailability = minimumAvailability;
        invalidate();
        Q_EMIT minimumAvailabilityChanged();
    }
}

QList<int> ProviderFilterModel::httpUploadFileSizes()
{
    return HTTP_UPLOAD_FILE_SIZES;
}

void ProviderFilterModel::setMinimumHttpUploadFileSize(int minimumHttpUploadFileSize)
{
    if (m_minimumHttpUploadFileSize != minimumHttpUploadFileSize) {
        m_minimumHttpUploadFileSize = minimumHttpUploadFileSize;
        invalidate();
        Q_EMIT minimumHttpUploadFileSizeChanged();
    }
}

QList<int> ProviderFilterModel::httpUploadTotalSizes()
{
    return HTTP_UPLOAD_TOTAL_SIZES;
}

void ProviderFilterModel::setMinimumHttpUploadTotalSize(int minimumHttpUploadTotalSize)
{
    if (m_minimumHttpUploadTotalSize != minimumHttpUploadTotalSize) {
        m_minimumHttpUploadTotalSize = minimumHttpUploadTotalSize;
        invalidate();
        Q_EMIT minimumHttpUploadTotalSizeChanged();
    }
}

QList<int> ProviderFilterModel::httpUploadStorageDurations()
{
    return HTTP_UPLOAD_STORAGE_DURATIONS;
}

void ProviderFilterModel::setMinimumHttpUploadStorageDuration(int minimumHttpUploadStorageDuration)
{
    if (m_minimumHttpUploadStorageDuration != minimumHttpUploadStorageDuration) {
        m_minimumHttpUploadStorageDuration = minimumHttpUploadStorageDuration;
        invalidate();
        Q_EMIT minimumHttpUploadStorageDurationChanged();
    }
}

QList<int> ProviderFilterModel::messageStorageDurations()
{
    return MESSAGE_STORAGE_DURATIONS;
}

void ProviderFilterModel::setMinimumMessageStorageDuration(int minimumMessageStorageDuration)
{
    if (m_minimumMessageStorageDuration != minimumMessageStorageDuration) {
        m_minimumMessageStorageDuration = minimumMessageStorageDuration;
        invalidate();
        Q_EMIT minimumMessageStorageDurationChanged();
    }
}

void ProviderFilterModel::setFreeOfChargeOnly(bool freeOfChargeOnly)
{
    if (m_freeOfChargeOnly != freeOfChargeOnly) {
        m_freeOfChargeOnly = freeOfChargeOnly;
        invalidate();
        Q_EMIT freeOfChargeOnlyChanged();
    }
}

#include "moc_ProviderFilterModel.cpp"
