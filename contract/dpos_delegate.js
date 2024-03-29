'use strict';

const configKey = 'dpos_config';

function loadObj(key){
    let data = Chain.load(key);
    if(data !== false){
        return JSON.parse(data);
    }

    return false;
}

function query(input_str){
    let input  = JSON.parse(input_str);

    let object = {};
    if(input.method !== undefined){
        let cfg = loadObj(configKey);
    	Utils.assert(cfg !== false, 'Failed to load configuration.');
        object = Chain.delegateQuery(cfg.logic_contract, input_str);
        Utils.assert(object.error === undefined && object.result !== undefined, 'Failed to query contract.');
    }
    else{
       	throw '<unidentified operation type>';
    }

    return object.result;
}

function main(input_str){
    let input = JSON.parse(input_str);

	if(input.method !== undefined && input.method !== 'init') {
        let cfg = loadObj(configKey);
		Utils.assert(cfg !== false, 'Failed to load configuration.');
		Chain.delegateCall(cfg.logic_contract, input_str);
	}
    else {
        throw '<undidentified operation type>';
    }
}

function init(input_str){
    let input = JSON.parse(input_str);
	Utils.assert(addressCheck(input.params.logic_contract), 'Invalid logic contract address');
    Chain.delegateCall(input.params.logic_contract, input_str);

    return true;
}
