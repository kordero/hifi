//
//  VoxelServer.h
//  voxel-server
//
//  Created by Brad Hefta-Gaub on 8/21/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __voxel_server__VoxelServer__
#define __voxel_server__VoxelServer__

#include <QStringList>
#include <QDateTime>
#include <QtCore/QCoreApplication>

#include <ThreadedAssignment.h>
#include <EnvironmentData.h>

#include "../octree/OctreeServer.h"

#include "VoxelServerConsts.h"

/// Handles assignments of type VoxelServer - sending voxels to various clients.
class VoxelServer : public OctreeServer {
public:
    VoxelServer(const QByteArray& packet);
    ~VoxelServer();

    bool wantSendEnvironments() const { return _sendEnvironments; }
    bool getSendMinimalEnvironment() const { return _sendMinimalEnvironment; }
    EnvironmentData* getEnvironmentData(int i) { return &_environmentData[i]; }
    int getEnvironmentDataCount() const { return sizeof(_environmentData)/sizeof(EnvironmentData); }

    // Subclasses must implement these methods
    virtual OctreeQueryNode* createOctreeQueryNode();
    virtual Octree* createTree();
    virtual unsigned char getMyNodeType() const { return NodeType::VoxelServer; }
    virtual PacketType getMyQueryMessageType() const { return PacketTypeVoxelQuery; }
    virtual const char* getMyServerName() const { return VOXEL_SERVER_NAME; }
    virtual const char* getMyLoggingServerTargetName() const { return VOXEL_SERVER_LOGGING_TARGET_NAME; }
    virtual const char* getMyDefaultPersistFilename() const { return LOCAL_VOXELS_PERSIST_FILE; }

    // subclass may implement these method
    virtual void beforeRun();
    virtual bool hasSpecialPacketToSend(const SharedNodePointer& node);
    virtual int sendSpecialPacket(const SharedNodePointer& node);


private:
    bool _sendEnvironments;
    bool _sendMinimalEnvironment;
    EnvironmentData _environmentData[3];
    unsigned char _tempOutputBuffer[MAX_PACKET_SIZE];
};

#endif // __voxel_server__VoxelServer__
