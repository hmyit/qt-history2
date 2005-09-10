/****************************************************************************
**
** Copyright (C) 2004-2005 Trolltech AS. All rights reserved.
**
** This file is part of the example classes of the Qt Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef PEERWIRECLIENT_H
#define PEERWIRECLIENT_H

class QHostAddress;
class QTimerEvent;

#include <QBitArray>
#include <QList>
#include <QTcpSocket>

class PeerWireClient : public QTcpSocket
{
    Q_OBJECT
public:
    enum PeerWireStateFlag {
        ChokingPeer = 0x1,
        InterestedInPeer = 0x2,
        ChokedByPeer = 0x4,
        PeerIsInterested = 0x8
    };
    Q_DECLARE_FLAGS(PeerWireState, PeerWireStateFlag)

    PeerWireClient(QObject *parent = 0);
    void initialize(const QByteArray &infoHash, const QByteArray &peerId,
                    int pieceCount);

    // State
    inline PeerWireState peerWireState() const { return pwState; }
    QBitArray availablePieces() const;

    // Protocol
    void chokePeer();
    void unchokePeer();
    void sendInterested();
    void sendNotInterested();
    void sendPieceNotification(int piece);
    void sendPieceList(const QBitArray &bitField);
    void requestBlock(int piece, int offset, int length);
    void cancelRequest(int piece, int offset, int length);
    void sendBlock(int piece, int offset, const QByteArray &data);

    // Rate control
    qint64 writeToSocket(qint64 bytes);
    qint64 readFromSocket(qint64 bytes);
    qint64 downloadSpeed() const;
    qint64 uploadSpeed() const;

    bool canTransferMore() const;
    inline qint64 bytesAvailable() const { return incomingBuffer.size(); }
    inline qint64 bytesToWrite() const { return outgoingBuffer.size() + QTcpSocket::bytesToWrite(); }

signals:
    void readyToTransfer();

    void choked();
    void unchoked();
    void interested();
    void notInterested();

    void piecesAvailable(const QBitArray &pieces);
    void blockRequested(int pieceIndex, int begin, int length);
    void requestCanceled(int pieceIndex, int begin, int length);
    void blockReceived(int pieceIndex, int begin, const QByteArray &data);

    void bytesReceived(qint64 size);

protected slots:
    void timerEvent(QTimerEvent *event);

protected:
    qint64 readData(char *data, qint64 maxlen);
    qint64 readLineData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);

private slots:
    void sendHandShake();
    void processIncomingData();
    void closeConnection();

private:
    // Data waiting to be read/written
    QByteArray incomingBuffer;
    QByteArray outgoingBuffer;
    QList<QByteArray> pendingBlocks;

    enum PacketType {
        ChokePacket = 0,
        UnchokePacket = 1,
        InterestedPacket = 2,
        NotInterestedPacket = 3,
        HavePacket = 4,
        BitFieldPacket = 5,
        RequestPacket = 6,
        PiecePacket = 7,
        CancelPacket = 8
    };

    // State
    PeerWireState pwState;
    bool receivedHandShake;
    bool gotPeerId;
    bool sentHandShake;
    int nextPacketLength;

    // Upload/download speed records
    qint64 uploadSpeedData[8];
    qint64 downloadSpeedData[8];
    int transferSpeedTimer;

    // Timeout handling
    int timeoutTimer;
    bool invalidateTimeout;

    // Checksum, peer ID and set of available pieces
    QByteArray infoHash;
    QByteArray peerIdString;
    QBitArray peerPieces;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(PeerWireClient::PeerWireState)

#endif
