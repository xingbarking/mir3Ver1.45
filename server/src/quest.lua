--, u8R"###(
--

function loadMap(map)
    return _RSVD_NAME_callFuncCoop('loadMap', map)
end

function getNPCharUID(mapName, npcName)
    local mapUID = loadMap(mapName)
    assertType(mapUID, 'integer')

    if mapUID == 0 then
        return 0
    end

    local npcUID = uidExecute(mapUID, [[
        return getNPCharUID('%s')
    ]], npcName)

    assertType(npcUID, 'integer')
    return npcUID
end

local _RSVD_NAME_triggers = {}
function addQuestTrigger(triggerType, callback)
    assertType(triggerType, 'integer')
    assertType(callback, 'function')

    assert(triggerType >= SYS_ON_BEGIN, triggerType)
    assert(triggerType <  SYS_ON_END  , triggerType)

    if not _RSVD_NAME_triggers[triggerType] then
        _RSVD_NAME_triggers[triggerType] = {}
        _RSVD_NAME_callFuncCoop('modifyQuestTriggerType', triggerType, true)
    end

    table.insert(_RSVD_NAME_triggers[triggerType], callback)
end

function _RSVD_NAME_trigger(triggerType, uid, ...)
    assertType(triggerType, 'integer')
    assertType(uid        , 'integer')

    assert(triggerType >= SYS_ON_BEGIN, triggerType)
    assert(triggerType <  SYS_ON_END  , triggerType)

    local args = {...}
    local config = _RSVD_NAME_triggerConfig(triggerType)

    if not config then
        fatalPrintf('Can not process trigger type %d', triggerType)
    end

    if #args > #config[2] then
        addLog(LOGTYPE_WARNING, 'Trigger type %s expected %d parameters, got %d', config[1], #config[2] + 1, #args + 1)
    end

    for i = 1, #config[2] do
        if type(args[i]) ~= config[2][i] and math.type(args[i]) ~= config[2][i] then
            fatalPrintf('The %d-th parmeter of trigger type %s expected %s, got %s', i + 1, config[1], config[2][i], type(args[i]))
        end
    end

    if config[3] then
        config[3](args)
    end

    local doneKeyList = {}
    for triggerKey, triggerFunc in pairs(_RSVD_NAME_triggers[triggerType]) do
        local result = triggerFunc(uid, table.unpack(args, 1, #config[2]))
        if type(result) == 'boolean' then
            if result then
                table.insert(doneKeyList, triggerKey)
            end
        elseif type(result) ~= 'nil' then
            table.insert(doneKeyList, triggerKey)
            addLog(LOGTYPE_WARNING, 'Trigger %s callback returns invalid type %s, key %s removed.', config[1], type(result), tostring(triggerKey))
        end
    end

    for _, key in ipairs(doneKeyList) do
        _RSVD_NAME_triggers[triggerType][key] = nil
    end

    if tableEmpty(_RSVD_NAME_triggers[triggerType]) then
        _RSVD_NAME_triggers[triggerType] = nil
        _RSVD_NAME_callFuncCoop('modifyQuestTriggerType', SYS_ON_KILL, false)
    end
end

--
-- )###"
