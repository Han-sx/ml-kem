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

# 读取合约 ABI（根据功能选择）
with open('Encrypt_data.json', 'r') as file:
    contract_abi = json.load(file)

# 替换为你的合约地址(需要修改)
contract_address = '0xBd8010BeAe0Cae88CDF5cfaAc1aFD05A67e71Fde'

contract = web3.eth.contract(address=contract_address, abi=contract_abi)

# 假设合约中有一个名为 getAllMlKemEncryptedDataWithIds 的只读函数，传入需要的参数(需要修改)
result = contract.functions.getAllMlKemEncryptedDataWithIds().call()
print('The value from the contract is:', result)

# # =========== 需要gas(执行提交数据) ===========
#
# 替换为你的以太坊账户私钥（需要修改）
private_key = '0x188d25fccbe599782378fbad0c7a03d7e1536d765df6b179077aa883f2803fb0'
account = Account.from_key(private_key)
print(account.address)

# 假设合约中有一个名为 storeMlKemEncryptedData 的可写函数，需要传入一个参数
hex_string = ''
# 从 ciphertext.txt 文件中读取加密数据
try:
    with open('ciphertext.txt', 'r') as file:
        # 读取文件内容
        hex_string = file.read().strip()
        # 确保十六进制字符串以 0x 开头
        if not hex_string.startswith('0x'):
            hex_string = '0x' + hex_string
    print(hex_string)
except FileNotFoundError:
    print("Error: ciphertext.txt file not found.")
    import sys
    sys.exit(1)


# 构建交易(需要修改)
transaction = contract.functions.storeMlKemEncryptedData(hex_string).build_transaction({
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
