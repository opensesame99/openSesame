/*
    This file is part of move-chain.

    move-chain is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    move-chain is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with move-chain.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file Common.cpp
 * @author Alex Leverington <nessence@gmail.com>
 * @author Gav Wood <i@gavwood.com>
 * @author Asherli
 * @date 2018
 */

#include "Common.h"
#include "AES.h"
#include "Exceptions.h"
#include "Hash.h"
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/pwdbased.h>
#include <cryptopp/sha.h>
#include <libdevcore/Guards.h>
#include <libdevcore/RLP.h>
#include <libethcore/Exceptions.h>
#include <secp256k1.h>
#include <secp256k1_recovery.h>
#include <secp256k1_sha256.h>
using namespace std;
using namespace dev;
using namespace dev::crypto;


static const u256 c_secp256k1n(
    "115792089237316195423570985008687907852837564279074904382605163141518161494337");

SignatureStruct::SignatureStruct(Signature const& _s)
{
    *(Signature*)this = _s;
}


SignatureStruct::SignatureStruct(h256 const& _r, h256 const& _s, VType _v) : r(_r), s(_s), v(_v) {}
SignatureStruct::SignatureStruct(u256 const& _r, u256 const& _s, NumberVType _v)
{
    r = _r;
    s = _s;
    v = _v;
}

pair<bool, bytes> SignatureStruct::ecRecover(bytesConstRef _in)
{
    struct
    {
        h256 hash;
        h256 v;
        h256 r;
        h256 s;
    } in;

    memcpy(&in, _in.data(), min(_in.size(), sizeof(in)));

    h256 ret;
    u256 v = (u256)in.v;
    if (v >= 27 && v <= 28)
    {
        SignatureStruct sig(in.r, in.s, (byte)((int)v - 27));
        if (sig.isValid())
        {
            try
            {
                if (Public rec = recover(sig, in.hash))
                {
                    ret = dev::sha3(rec);
                    memset(ret.data(), 0, 12);
                    return {true, ret.asBytes()};
                }
            }
            catch (...)
            {
            }
        }
    }
    return {true, {}};
}

void SignatureStruct::encode(RLPStream& _s) const noexcept
{
    _s << (VType)(v + VBase) << (u256)r << (u256)s;
}


void SignatureStruct::check() const noexcept
{
    if (s > c_secp256k1n / 2)
        BOOST_THROW_EXCEPTION(eth::InvalidSignature());
}


namespace
{
/**
 * @brief : init secp256k1_context globally(maybe for secure consider)
 * @return secp256k1_context const* : global static secp256k1_context
 */
secp256k1_context const* getCtx()
{
    static std::unique_ptr<secp256k1_context, decltype(&secp256k1_context_destroy)> s_ctx{
        secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY),
        &secp256k1_context_destroy};
    return s_ctx.get();
}


}  // namespace

bool dev::SignatureStruct::isValid() const noexcept
{
    static const h256 s_max{"0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141"};
    static const h256 s_zero;

    return (v <= 1 && r > s_zero && s > s_zero && r < s_max && s < s_max);
}

/**
 * @brief : obtain public key according to secret key
 * @param _secret : the data of secret key
 * @return Public : created public key; if create failed, assertion failed
 */
Public dev::toPublic(Secret const& _secret)
{
    auto* ctx = getCtx();
    secp256k1_pubkey rawPubkey;
    // Creation will fail if the secret key is invalid.
    if (!secp256k1_ec_pubkey_create(ctx, &rawPubkey, _secret.data()))
        return {};
    std::array<byte, 65> serializedPubkey;
    size_t serializedPubkeySize = serializedPubkey.size();
    secp256k1_ec_pubkey_serialize(
        ctx, serializedPubkey.data(), &serializedPubkeySize, &rawPubkey, SECP256K1_EC_UNCOMPRESSED);
    assert(serializedPubkeySize == serializedPubkey.size());
    // Expect single byte header of value 0x04 -- uncompressed public key.
    assert(serializedPubkey[0] == 0x04);
    // Create the Public skipping the header.
    return Public{&serializedPubkey[1], Public::ConstructFromPointer};
}

/**
 * @brief obtain address from public key
 *        by adding the last 20Bytes of sha3(public key)
 * @param _public : the public key need to convert to address
 * @return Address : the converted address
 */
Address dev::toAddress(Public const& _public)
{
    return right160(sha3(_public.ref()));
}

Address dev::toAddress(Secret const& _secret)
{
    return toAddress(toPublic(_secret));
}

/**
 * @brief : 1.serialize (_from address, nonce) into rlpStream
 *          2.calculate the sha3 of serialized (_from, nonce)
 *          3.obtaining the last 20Bytes of the sha3 as address
 *          (mainly used for contract address generating)
 * @param _from : address that sending this transaction
 * @param _nonce : random number
 * @return Address : generated address
 */
Address dev::toAddress(Address const& _from, u256 const& _nonce)
{
    return right160(sha3(rlpList(_from, _nonce)));
}

Public dev::recover(Signature const& _sig, h256 const& _message)
{
    int v = _sig[64];
    if (v > 3)
        return {};

    auto* ctx = getCtx();
    secp256k1_ecdsa_recoverable_signature rawSig;
    if (!secp256k1_ecdsa_recoverable_signature_parse_compact(ctx, &rawSig, _sig.data(), v))
        return {};

    secp256k1_pubkey rawPubkey;
    if (!secp256k1_ecdsa_recover(ctx, &rawPubkey, &rawSig, _message.data()))
        return {};

    std::array<byte, 65> serializedPubkey;
    size_t serializedPubkeySize = serializedPubkey.size();
    secp256k1_ec_pubkey_serialize(
        ctx, serializedPubkey.data(), &serializedPubkeySize, &rawPubkey, SECP256K1_EC_UNCOMPRESSED);
    assert(serializedPubkeySize == serializedPubkey.size());
    // Expect single byte header of value 0x04 -- uncompressed public key.
    assert(serializedPubkey[0] == 0x04);
    // Create the Public skipping the header.
    return Public{&serializedPubkey[1], Public::ConstructFromPointer};
}


Signature dev::sign(Secret const& _k, h256 const& _hash)
{
    auto* ctx = getCtx();
    secp256k1_ecdsa_recoverable_signature rawSig;
    if (!secp256k1_ecdsa_sign_recoverable(ctx, &rawSig, _hash.data(), _k.data(), nullptr, nullptr))
        return {};

    Signature s;
    int v = 0;
    secp256k1_ecdsa_recoverable_signature_serialize_compact(ctx, s.data(), &v, &rawSig);

    SignatureStruct& ss = *reinterpret_cast<SignatureStruct*>(&s);
    ss.v = static_cast<byte>(v);
    if (ss.s > c_secp256k1n / 2)
    {
        ss.v = static_cast<byte>(ss.v ^ 1);
        ss.s = h256(c_secp256k1n - u256(ss.s));
    }
    assert(ss.s <= c_secp256k1n / 2);
    return s;
}

bool dev::verify(Public const& _p, Signature const& _s, h256 const& _hash)
{
    // TODO: Verify w/o recovery (if faster).
    if (!_p)
        return false;
    return _p == recover(_s, _hash);
}


KeyPair KeyPair::create()
{
    KeyPair keyPair(Secret::random());
    return keyPair;
}


Secret Nonce::next()
{
    Guard l(x_value);
    if (!m_value)
    {
        m_value = Secret::random();
        if (!m_value)
            BOOST_THROW_EXCEPTION(InvalidState());
    }
    m_value = sha3Secure(m_value.ref());
    return sha3(~m_value);
}
