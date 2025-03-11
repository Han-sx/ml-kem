import pprint
import time
from web3 import Web3
import json
from eth_account import Account

# 连接以太坊节点
# 替换为你自己的项目的网络节点 URL（需要修改）
infura_url = 'HTTP://192.168.50.103:7545'
web3 = Web3(Web3.HTTPProvider(infura_url))

# 检查是否成功连接到网络
if web3.is_connected():
    print('Connected to the Sepolia testnet')
else:
    print('Failed to connect to the Sepolia testnet')

# 读取合约 ABI（需要修改，根据功能选择）
with open('Random.json', 'r') as file:
    contract_abi = json.load(file)

# 替换为你的合约地址(需要修改)
contract_address = '0x6914a2F37D50B1bC4C3184c6BBbb984343EcCE43'

contract = web3.eth.contract(address=contract_address, abi=contract_abi)

# 假设合约中有一个名为 currentId 的只读函数，传入需要的参数(需要修改)
result = contract.functions.currentId().call()
print('The value from the contract is:', result)

# # =========== 需要gas(执行提交数据) ===========
#
# 替换为你的以太坊账户私钥（需要修改）
private_key = '0x188d25fccbe599782378fbad0c7a03d7e1536d765df6b179077aa883f2803fb0'
account = Account.from_key(private_key)
print(account.address)

# 假设合约中有一个名为 generateRandomNumber 的可写函数
# 构建交易(需要修改)
transaction = contract.functions.generateRandomNumber().build_transaction({
    'from': account.address,
    'nonce': web3.eth.get_transaction_count(account.address)
})

# # 签名交易
signed_set_txn = web3.eth.account.sign_transaction(transaction, private_key)
print(signed_set_txn)
# 发送交易
tx_hash = web3.eth.send_raw_transaction(signed_set_txn.raw_transaction)

# 等待交易确认
receipt = web3.eth.wait_for_transaction_receipt(tx_hash)
# 使用 pprint 格式化输出交易收据
pp = pprint.PrettyPrinter(indent=4)
print('Transaction receipt:')
pp.pprint(dict(receipt))

# 获取随机数
# 假设合约中有一个名为 getRandomNumberById 的只读函数，传入需要的参数(需要修改)
result_2 = contract.functions.getRandomNumberById(result).call()
print('The value from the contract is:', result_2)

# 将字符串编码为字节对象
random_number_bytes = str(result_2).encode('utf-8')

# 读取 data_file.dat 文件的内容
try:
    with open('data_file.dat', 'rb') as file:
        file_content = file.read()
except FileNotFoundError:
    print("data_file.dat 文件未找到。")
    file_content = b''

# 将随机数添加到文件内容的最前面
new_content = random_number_bytes + file_content

# 将拼接后的内容写回 data_file.dat 文件
with open('data_file.dat', 'wb') as file:
    file.write(new_content)

print("随机数已成功添加到 data_file.dat 文件的最前面。")