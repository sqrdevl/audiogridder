/*
 * Copyright (c) 2020 Andreas Pohl
 * Licensed under MIT (https://github.com/apohl79/audiogridder/blob/master/COPYING)
 *
 * Author: Andreas Pohl
 */

#ifndef Utils_hpp
#define Utils_hpp

#ifdef AG_SERVER

#define getApp() dynamic_cast<App*>(JUCEApplication::getInstance())

#if (JUCE_DEBUG && !JUCE_DISABLE_ASSERTIONS)
#define dbgln(M) \
    JUCE_BLOCK_WITH_FORCED_SEMICOLON(String __str; __str << "[" << getLogTag() << "] " << M; Logger::writeToLog(__str);)
#else
#define dbgln(M)
#endif

#define logln(M) \
    JUCE_BLOCK_WITH_FORCED_SEMICOLON(String __str; __str << "[" << getLogTag() << "] " << M; Logger::writeToLog(__str);)

#else

#include "Logger.hpp"

#if JUCE_DEBUG
#define dbgln(M) \
    JUCE_BLOCK_WITH_FORCED_SEMICOLON(String __str; __str << "[" << getLogTag() << "] " << M; AGLogger::log(__str);)
#else
#define dbgln(M)
#endif

#define logln(M) \
    JUCE_BLOCK_WITH_FORCED_SEMICOLON(String __str; __str << "[" << getLogTag() << "] " << M; AGLogger::log(__str);)

#endif

#define setLogTagStatic(t) auto getLogTag = [] { return LogTag::getTaggedStr(t, "static"); };

namespace e47 {

class LogTag {
  public:
    LogTag(const String& name) : m_name(name) {}

    static String getStrWithLeadingZero(int n, int digits = 2) {
        String s = "";
        while (--digits > 0) {
            if (n < pow(10, digits)) {
                s << "0";
            }
        }
        s << n;
        return s;
    }

    static String getTimeStr() {
        auto now = Time::getCurrentTime();
        auto H = getStrWithLeadingZero(now.getHours());
        auto M = getStrWithLeadingZero(now.getMinutes());
        auto S = getStrWithLeadingZero(now.getSeconds());
        auto m = getStrWithLeadingZero(now.getMilliseconds(), 3);
        String ret = "";
        ret << H << ":" << M << ":" << S << "." << m;
        return ret;
    }

    static String getTaggedStr(const String& name, const String& ptr) {
        String tag = "";
        tag << getTimeStr() << "|" << name << "|" << ptr;
        return tag;
    }

    String getLogTag() const { return getTaggedStr(m_name, String((uint64)this)); }

  private:
    String m_name;
};

class LogTagDelegate {
  public:
    LogTagDelegate() {}
    LogTagDelegate(LogTag* r) : m_logTagSrc(r) {}
    LogTag* m_logTagSrc;
    void setLogTagSource(LogTag* r) { m_logTagSrc = r; }
    LogTag* getLogTagSource() const { return m_logTagSrc; }
    String getLogTag() const {
        if (nullptr != m_logTagSrc) {
            return m_logTagSrc->getLogTag();
        }
        return "";
    }
};

static inline void waitForThreadAndLog(LogTag* tag, Thread* t, int millisUntilWarning = 3000) {
    auto getLogTag = [tag] { return tag->getLogTag(); };
    if (millisUntilWarning > -1) {
        auto warnTime = Time::getMillisecondCounter() + (uint32)millisUntilWarning;
        while (!t->waitForThreadToExit(1000)) {
            if (Time::getMillisecondCounter() > warnTime) {
                logln("warning: waiting for thread " << t->getThreadName() << " to finish");
            }
        }
    } else {
        t->waitForThreadToExit(-1);
    }
}

class ServerInfo {
  public:
    ServerInfo() {
        m_id = 0;
        m_load = 0.0f;
        refresh();
    }

    ServerInfo(const String& s) {
        auto hostParts = StringArray::fromTokens(s, ":", "");
        if (hostParts.size() > 1) {
            m_host = hostParts[0];
            m_id = hostParts[1].getIntValue();
            if (hostParts.size() > 2) {
                m_name = hostParts[2];
            }
        } else {
            m_host = s;
            m_id = 0;
        }
        m_load = 0.0f;
        refresh();
    }

    ServerInfo(const String& host, const String& name, int id, float load)
        : m_host(host), m_name(name), m_id(id), m_load(load) {
        refresh();
    }

    ServerInfo(const ServerInfo& other)
        : m_host(other.m_host), m_name(other.m_name), m_id(other.m_id), m_load(other.m_load) {
        refresh();
    }

    bool operator==(const ServerInfo& other) const {
        return m_host == other.m_host && m_name == other.m_name && m_id == other.m_id;
    }

    const String& getHost() const { return m_host; }
    const String& getName() const { return m_name; }
    int getID() const { return m_id; }
    float getLoad() const { return m_load; }

    String getHostAndID() const {
        String ret = m_host;
        if (m_id > 0) {
            ret << ":" << m_id;
        }
        return ret;
    }

    String getNameAndID() const {
        String ret = m_name;
        if (m_id > 0) {
            ret << ":" << m_id;
        }
        return ret;
    }

    String toString() const {
        String ret = "Server(";
        ret << "name=" << m_name << ", ";
        ret << "host=" << m_host << ", ";
        ret << "id=" << m_id;
        if (m_load > 0.0f) {
            ret << ", load=" << m_load;
        }
        ret << ")";
        return ret;
    }

    String serialize() const {
        String ret = m_host;
        ret << ":" << m_id << ":" << m_name;
        return ret;
    }

    Time getUpdated() const { return m_updated; }

    void refresh() { m_updated = Time::getCurrentTime(); }

    void refresh(float load) {
        refresh();
        m_load = load;
    }

  private:
    String m_host, m_name;
    int m_id;
    float m_load;
    Time m_updated;
};

inline void callOnMessageThread(std::function<void()> fn) {
    std::mutex mtx;
    std::condition_variable cv;
    bool done = false;
    MessageManager::callAsync([&] {
        std::lock_guard<std::mutex> lock(mtx);
        fn();
        done = true;
        cv.notify_one();
    });
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [&done] { return done; });
}

}  // namespace e47

#endif /* Utils_hpp */
