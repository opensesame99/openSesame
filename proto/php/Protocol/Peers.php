<?php
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: overlay.proto

namespace Protocol;

use Google\Protobuf\Internal\GPBType;
use Google\Protobuf\Internal\RepeatedField;
use Google\Protobuf\Internal\GPBUtil;

/**
 * Generated from protobuf message <code>protocol.Peers</code>
 */
class Peers extends \Google\Protobuf\Internal\Message
{
    /**
     * Generated from protobuf field <code>repeated .protocol.Peer peers = 1;</code>
     */
    private $peers;

    public function __construct() {
        \GPBMetadata\Overlay::initOnce();
        parent::__construct();
    }

    /**
     * Generated from protobuf field <code>repeated .protocol.Peer peers = 1;</code>
     * @return \Google\Protobuf\Internal\RepeatedField
     */
    public function getPeers()
    {
        return $this->peers;
    }

    /**
     * Generated from protobuf field <code>repeated .protocol.Peer peers = 1;</code>
     * @param \Protocol\Peer[]|\Google\Protobuf\Internal\RepeatedField $var
     * @return $this
     */
    public function setPeers($var)
    {
        $arr = GPBUtil::checkRepeatedField($var, \Google\Protobuf\Internal\GPBType::MESSAGE, \Protocol\Peer::class);
        $this->peers = $arr;

        return $this;
    }

}

