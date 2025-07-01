#pragma once

#include <string>
#include <vector>

// 定义一个纯虚接口，用户需要实现这个接口来处理接收到的数据
class serverCallback
{
public:
    virtual ~serverCallback() = default;

    /**
     * @brief Interface for server callback handling.
     *
     * @param client_id (int) The ID of the client that sent the data.
     *
     * This interface should be implemented by users who want to handle events from the server,
     * such as when data is received from a client. Implementations must provide logic for
     * processing incoming data and can define additional event handling methods as needed.
     */
    virtual void onDataReceived(int client_id) = 0;
};