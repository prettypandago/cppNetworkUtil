import requests
import time
import sys
import os 
from concurrent.futures import ThreadPoolExecutor

# ----------------------------------------------------
# 导入 requests 库，并引入 urllib3 用于禁用 SSL 警告
# ----------------------------------------------------
try:
    # 禁用 SSL 警告
    requests.packages.urllib3.disable_warnings()
except AttributeError:
    print("警告：无法禁用 SSL 警告。")
except ImportError:
    print("错误：请安装 requests 库 (pip install requests)")
    sys.exit(1)


# --- 默认配置和常量 (使用下划线分割命名风格) ---
DEFAULT_TARGET_ADDRESS = "127.0.0.1"
DEFAULT_TARGET_PORT = 443
DEFAULT_REQUESTS_PER_CLIENT = 100 # 每个客户端的请求次数
DEFAULT_CONCURRENT_CLIENTS = 10  # 并发客户端数量
DEFAULT_MIN_MS = 2.0
DEFAULT_MAX_MS = 7.0 
TIMEOUT_SECONDS = 5
EXPECTED_SUBSTRING = "" # ⚠️ 移除内容检查后，此常量实际上已无用
# ----------------------------------------------------

def get_user_input():
    """获取用户输入的所有测试参数和预期的性能要求，并移除 X, Y 标识"""
    
    if os.name == 'nt':
        os.system('cls')
    else:
        os.system('clear')

    print("--- 🚀 C++ HTTPS 并发性能测试工具 ---")
    
    # 1. 获取目标地址
    target_address = input(f"请输入目标地址 (IP/域名, 默认: {DEFAULT_TARGET_ADDRESS}): ").strip() or DEFAULT_TARGET_ADDRESS

    # 2. 获取目标端口
    while True:
        try:
            target_port = int(input(f"请输入目标端口 (默认: {DEFAULT_TARGET_PORT}): ").strip() or DEFAULT_TARGET_PORT)
            if 1 <= target_port <= 65535: break
            print("端口号必须在 1 到 65535 之间。请重新输入。")
        except ValueError:
            print("输入无效。端口号必须是整数。请重新输入。")

    # 3. 获取并发客户端数量
    while True:
        try:
            concurrent_clients = int(input(f"请输入并发客户端数量 (默认: {DEFAULT_CONCURRENT_CLIENTS}, 至少1): ").strip() or DEFAULT_CONCURRENT_CLIENTS)
            if concurrent_clients >= 1: break
            print("客户端数量必须至少为 1。请重新输入。")
        except ValueError:
            print("输入无效。客户端数量必须是整数。请重新输入。")

    # 4. 获取每个客户端的请求次数
    while True:
        try:
            # 保持至少 2 次的检查，因为它现在包含请求 1
            requests_per_client = int(input(f"请输入每个客户端的请求次数 (默认: {DEFAULT_REQUESTS_PER_CLIENT}, 至少2次): ").strip() or DEFAULT_REQUESTS_PER_CLIENT)
            if requests_per_client >= 1: # 实际只需要至少1次
                break
            print("请求次数必须至少为 1 次。请重新输入。")
        except ValueError:
            print("输入无效。请求次数必须是整数。请重新输入。")
            
    # 5. 获取预期最小延迟
    while True:
        try:
            expected_min_ms = float(input(f"请输入预期最小延迟 (Keep-Alive, 默认: {DEFAULT_MIN_MS:.1f} ms): ").strip() or DEFAULT_MIN_MS)
            break
        except ValueError:
            print("输入无效。最小延迟必须是数字。请重新输入。")
            
    # 6. 获取预期最大延迟
    while True:
        try:
            expected_max_ms = float(input(f"请输入预期最大延迟 (Keep-Alive, 默认: {DEFAULT_MAX_MS:.1f} ms): ").strip() or DEFAULT_MAX_MS)
            if expected_max_ms < expected_min_ms:
                print("最大延迟不能小于最小延迟。请重新输入。")
            else:
                break
        except ValueError:
            print("输入无效。最大延迟必须是数字。请重新输入。")
            
    print("-" * 40)
    return target_address, target_port, concurrent_clients, requests_per_client, expected_min_ms, expected_max_ms


def check_initial_connection(url):
    """
    进行连接检查和预热，确保服务器可达且响应状态码正确。
    此函数现在会正常计时并打印首个请求的真实耗时。
    """
    print("--- 1. 建立连接检查和预热 (确保服务器可达) ---")
    try:
        session = requests.Session()
        session.verify = False 
        
        start_time = time.perf_counter()
        response = session.get(url, timeout=TIMEOUT_SECONDS)
        end_time = time.perf_counter()
        
        latency_ms = (end_time - start_time) * 1000
        
        # 核心修改 1：移除内容检查，只检查状态码
        if response.status_code == 200:
            print(f"  [检查通过] 状态码: 200, 内容: '{response.text[:20].strip()}...'")
            print(f"  [检查通过] 首个请求耗时 (ms): {latency_ms:.3f}\n") # 打印真实耗时
        else:
            print(f"  [检查失败] 状态码: {response.status_code}, 内容: '{response.text.strip()}'")
            print("  **错误: 服务器响应状态码不为 200。**")
            sys.exit(1)
            
    except requests.exceptions.RequestException as e:
        print(f"❌ 致命错误: 无法连接到服务器。请检查参数和服务器状态。错误信息: {e}")
        sys.exit(1)


def perform_client_requests(client_id, url, requests_per_client):
    """
    单个客户端的测试逻辑。从第一个请求（包含连接）开始计时。
    
    返回: (客户端ID, 该客户端所有请求的延迟列表)。
    """
    session = requests.Session()
    session.verify = False
    latency_records = []
    
    # 核心修改 2：从请求 1 开始循环，取代了原有的不计时预热
    for i in range(1, requests_per_client + 1):
        try:
            start_time = time.perf_counter()
            response = session.get(url, timeout=TIMEOUT_SECONDS)
            end_time = time.perf_counter()
            
            latency_ms = (end_time - start_time) * 1000
            
            # 核心修改 3：移除内容检查
            if response.status_code != 200:
                # 记录错误并停止该客户端
                latency_records.append(("Error", response.status_code))
                break 
                
            latency_records.append((i, latency_ms))

        except requests.exceptions.RequestException:
            # 网络中断或其他 I/O 错误
            break
            
    return client_id, latency_records


def run_concurrent_test(url, requests_per_client, concurrent_clients, expected_max_ms):
    """使用线程池启动并发测试，并打印详细结果"""
    print(f"--- 2. 启动并发测试 ({concurrent_clients} 客户端 X {requests_per_client} 请求) ---")
    
    # 存储所有客户端的结果 (客户端ID -> 延迟列表)
    results_map = {}
    
    # 创建线程池
    with ThreadPoolExecutor(max_workers=concurrent_clients) as executor:
        # 提交所有客户端的任务
        futures = {executor.submit(perform_client_requests, i + 1, url, requests_per_client) 
                   for i in range(concurrent_clients)}
        
        # 收集结果
        for future in futures:
            client_id, latency_list = future.result()
            results_map[client_id] = latency_list

    # 打印详细的客户端分组结果
    all_latency_values = []
    
    print("\n--- 客户端详细响应时间 (Keep-Alive) ---")
    
    # 按客户端ID排序打印
    for client_id in sorted(results_map.keys()):
        latency_list = results_map[client_id]
        
        # 核心修改 4：更新打印信息，反映包含首次连接
        print(f"\n>>>> 客户端 {client_id:02d} (共 {len(latency_list)} 个请求，包括首次连接)")

        # 打印每个请求的详细耗时
        for item in latency_list:
            if item[0] == "Error":
                 print(f"  请求失败 (状态码 {item[1]})")
                 continue
            
            request_index, latency_ms = item
            all_latency_values.append(latency_ms)
            
            if latency_ms > expected_max_ms:
                 # 高亮超出预期的耗时
                 print(f"  请求 {request_index:03d}: 状态码 200, 耗时: **{latency_ms:.3f} ms (⚠️)**")
            else:
                 print(f"  请求 {request_index:03d}: 状态码 200, 耗时: {latency_ms:.3f} ms")

    return all_latency_values


def summarize_results(all_latency_records, expected_min_ms, expected_max_ms):
    """汇总并评估测试结果"""
    print("\n" + "=" * 40)
    print("--- 3. 最终结果汇总 ---")
    
    if not all_latency_records:
        print("没有收集到有效的延迟数据。测试可能失败。")
        return

    min_latency = min(all_latency_records)
    max_latency = max(all_latency_records)
    avg_latency = sum(all_latency_records) / len(all_latency_records)
    total_requests = len(all_latency_records)
    
    # 计算超出最大延迟的请求数 (抖动)
    outliers_count = sum(1 for ms in all_latency_records if ms > expected_max_ms)

    print(f"总有效测试请求数: {total_requests}")
    print(f"预期性能目标: {expected_min_ms:.3f} ms - {expected_max_ms:.3f} ms")
    print("-" * 40)
    print(f"最小延迟 (Min): **{min_latency:.3f} ms**")
    print(f"最大延迟 (Max): **{max_latency:.3f} ms**")
    print(f"平均延迟 (Avg): **{avg_latency:.3f} ms**")
    print("-" * 40)
    
    print("\n**性能评估 (基于用户目标):**")
    
    if max_latency <= expected_max_ms:
        print("🎉 **完美！** 所有请求的延迟都未超出您的最大目标。高并发下表现稳定。")
    else:
        outlier_percentage = outliers_count/total_requests * 100
        print(f"⚠️ **存在抖动！** 有 {outliers_count} 个请求 ({outlier_percentage:.2f}%) 超过了您的最大目标 {expected_max_ms:.3f} ms。")
        print("请根据上方输出的详细结果，检查这些超时的请求主要出现在哪个客户端或请求序列中。")


if __name__ == "__main__":
    # 获取用户自定义参数
    target_address, target_port, concurrent_clients, requests_per_client, expected_min_ms, expected_max_ms = get_user_input()
    
    target_url = f"https://{target_address}:{target_port}/"
    
    # 步骤 1: 检查连接
    check_initial_connection(target_url)
    
    # 步骤 2: 运行并发测试并获取所有延迟数据
    all_latency_records = run_concurrent_test(target_url, requests_per_client, concurrent_clients, expected_max_ms)
    
    # 步骤 3: 汇总结果
    summarize_results(all_latency_records, expected_min_ms, expected_max_ms)