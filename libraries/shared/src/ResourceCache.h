//
//  ResourceCache.h
//  shared
//
//  Created by Andrzej Kapolka on 2/27/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __shared__ResourceCache__
#define __shared__ResourceCache__

#include <QHash>
#include <QList>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QPointer>
#include <QSharedPointer>
#include <QUrl>
#include <QWeakPointer>

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

class Resource;

/// Base class for resource caches.
class ResourceCache : public QObject {
    Q_OBJECT
    
public:

    static void setNetworkAccessManager(QNetworkAccessManager* manager) { _networkAccessManager = manager; }
    static QNetworkAccessManager* getNetworkAccessManager() { return _networkAccessManager; }

    static void setRequestLimit(int limit) { _requestLimit = limit; }
    static int getRequestLimit() { return _requestLimit; }

    ResourceCache(QObject* parent = NULL);
    virtual ~ResourceCache();

protected:

    QMap<int, QSharedPointer<Resource> > _unusedResources;

    /// Loads a resource from the specified URL.
    /// \param fallback a fallback URL to load if the desired one is unavailable
    /// \param delayLoad if true, don't load the resource immediately; wait until load is first requested
    /// \param extra extra data to pass to the creator, if appropriate
    QSharedPointer<Resource> getResource(const QUrl& url, const QUrl& fallback = QUrl(),
        bool delayLoad = false, void* extra = NULL);

    /// Creates a new resource.
    virtual QSharedPointer<Resource> createResource(const QUrl& url,
        const QSharedPointer<Resource>& fallback, bool delayLoad, const void* extra) = 0;

    void addUnusedResource(const QSharedPointer<Resource>& resource);
    
    static void attemptRequest(Resource* resource);
    static void requestCompleted();

private:
    
    friend class Resource;

    QHash<QUrl, QWeakPointer<Resource> > _resources;
    int _lastLRUKey;
    
    static QNetworkAccessManager* _networkAccessManager;
    static int _requestLimit;
    static QList<QPointer<Resource> > _pendingRequests;
};

/// Base class for resources.
class Resource : public QObject {
    Q_OBJECT

public:
    
    Resource(const QUrl& url, bool delayLoad = false);
    ~Resource();
    
    /// Returns the key last used to identify this resource in the unused map.
    int getLRUKey() const { return _lruKey; }

    /// Makes sure that the resource has started loading.
    void ensureLoading();

    /// Sets the load priority for one owner.
    virtual void setLoadPriority(const QPointer<QObject>& owner, float priority);
    
    /// Sets a set of priorities at once.
    virtual void setLoadPriorities(const QHash<QPointer<QObject>, float>& priorities);
    
    /// Clears the load priority for one owner.
    virtual void clearLoadPriority(const QPointer<QObject>& owner);
    
    /// Returns the highest load priority across all owners.
    float getLoadPriority();

    /// Checks whether the resource has loaded.
    bool isLoaded() const { return _loaded; }

    void setSelf(const QWeakPointer<Resource>& self) { _self = self; }

    void setCache(ResourceCache* cache) { _cache = cache; }

    void allReferencesCleared();

protected slots:

    void attemptRequest();

protected:

    /// Called when the download has finished.  The recipient should delete the reply when done with it.
    virtual void downloadFinished(QNetworkReply* reply) = 0;

    /// Should be called by subclasses when all the loading that will be done has been done.
    Q_INVOKABLE void finishedLoading(bool success);

    /// Reinserts this resource into the cache.
    virtual void reinsert();

    QUrl _url;
    QNetworkRequest _request;
    bool _startedLoading;
    bool _failedToLoad;
    bool _loaded;
    QHash<QPointer<QObject>, float> _loadPriorities;
    QWeakPointer<Resource> _self;
    QPointer<ResourceCache> _cache;
    
private slots:
    
    void handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void handleReplyError();
    void handleReplyFinished();
    void handleReplyTimeout();

private:
    
    void setLRUKey(int lruKey) { _lruKey = lruKey; }
    
    void makeRequest();
    
    void handleReplyError(QNetworkReply::NetworkError error, QDebug debug);
    
    friend class ResourceCache;
    
    int _lruKey;
    QNetworkReply* _reply;
    QTimer* _replyTimer;
    qint64 _bytesReceived;
    qint64 _bytesTotal;
    int _attempts;
};

uint qHash(const QPointer<QObject>& value, uint seed = 0);

#endif /* defined(__shared__ResourceCache__) */
