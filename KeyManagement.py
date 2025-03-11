import time
from web3 import Web3
import json
from eth_account import Account

# 连接以太坊节点
# 替换为你自己的项目的网络节点 URL（需要修改）
infura_url = 'HTTP://127.0.0.1:7545'
web3 = Web3(Web3.HTTPProvider(infura_url))

# 检查是否成功连接到网络
if web3.is_connected():
    print('Connected to the Sepolia testnet')
else:
    print('Failed to connect to the Sepolia testnet')

# 读取合约 ABI
with open('KeyManagement.json', 'r') as file:
    contract_abi = json.load(file)

# 替换为你的合约地址(需要修改)
contract_address = '0x0467BC95627fcb6A0C599BAC44C9CF02c655A6B6'

contract = web3.eth.contract(address=contract_address, abi=contract_abi)

# 假设合约中有一个名为 getPublicKey 的只读函数，传入需要的参数(需要修改)
user_addr = "0xC57368C82a78F6e4486D7C6d709139fB97bcAbaa"
result = contract.functions.owner().call()
print('The value from the contract is:', result)


# # =========== 需要gas ===========
#
# 替换为你的以太坊账户私钥（需要修改）
private_key = '0x188d25fccbe599782378fbad0c7a03d7e1536d765df6b179077aa883f2803fb0'
account = Account.from_key(private_key)
print(account.address)

# 假设合约中有一个名为 storeUserPublicKey 的可写函数，需要传入一个参数
hex_string = '0x123456'
print(hex_string)

# 构建交易(需要修改)
transaction = contract.functions.storeUserPublicKey(account.address, hex_string).build_transaction({
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
print('Transaction receipt:', tx_hash)

