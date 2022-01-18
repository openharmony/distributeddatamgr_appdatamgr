/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import {describe, beforeAll, beforeEach, afterEach, afterAll, it, expect} from 'deccjsunit/index'
import dataRdb from '@ohos.data.rdb';

const TAG = "[RDB_JSKITS_TEST_Distributed]"
const STORE_NAME = "distributed_rdb.db"
var rdbStore = undefined;

describe('rdbStoreDistributedTest', function () {
    beforeAll(async function () {
        console.info(TAG + 'beforeAll')
    })

    beforeEach(async function () {
        console.info(TAG + 'beforeEach')
    })

    afterEach(async function () {
        console.info(TAG + 'afterEach')
    })

    afterAll(async function () {
        console.info(TAG + 'afterAll')
        await dataRdb.deleteRdbStore(STORE_NAME);
    })

    console.log(TAG + "*************Unit Test Begin*************");

    /**
     * @tc.name rdb open test
     * @tc.number SUB_DDM_AppDataFWK_JSRDB_Distributed_001
     * @tc.desc rdb open test
     */
    it('testRdbStoreDistributed0001', 0, async function (done) {
        console.log(TAG + "************* testRdbStoreDistributed001 start *************");
        const config = {
            "name": STORE_NAME,
        }
        let promise = dataRdb.getRdbStore(config, 1);
        promise.then((store) => {
            console.log(TAG + "create rdb store success")
            rdbStore = store
            expect(rdbStore).assertEqual(rdbStore)

            let sqlStatement = "CREATE TABLE IF NOT EXISTS employee (" +
                "id INTEGER PRIMARY KEY AUTOINCREMENT," +
                "name TEXT NOT NULL," +
                "age INTEGER," +
                "salary REAL," +
                "data BLOB)"
            let promise1 = rdbStore.executeSql(sqlStatement, null)
            promise1.then(() => {
                console.log(TAG + "create table employee success")
            }).catch(() => {
                console.log(TAG + "create table employee failed")
                expect(null).assertFail()
            })

            sqlStatement = "CREATE TABLE IF NOT EXISTS product (" +
                "id INTEGER PRIMARY KEY AUTOINCREMENT," +
                "name TEXT NOT NULL," +
                "price REAL," +
                "vendor INTEGER," +
                "describe TEXT)"
            let promise2 = rdbStore.executeSql(sqlStatement, null)
            promise2.then(() => {
                console.log(TAG + "create table product success")
            }).catch(() => {
                console.log(TAG + "create table product failed")
                expect(null).assertFail()
            })
        }).catch((error) => {
            console.log(TAG + "create rdb store failed")
            expect(null).assertFail()
        })
        done()
        console.log(TAG + "************* testRdbStoreDistributed001 end *************");
    })

    /**
     * @tc.name set_distributed_table_none_table
     * @tc.number SUB_DDM_AppDataFWK_JSRDB_Distributed_002
     * @tc.desc rdb set distributed table using none table as argment
     */
    it('testRdbStoreDistributed0002', 0, async function (done) {
        console.log(TAG + "************* testRdbStoreDistributed002 start *************");
        let promise = rdbStore.setDistributedTables([])
        promise.then(() => {
            console.log(TAG + "set none to be distributed table success");
            expect(rdbStore).assertEqual(rdbStore)
        }).catch(() => {
            console.log(TAG + "set none to be distributed table failed");
            expect(null).assertFail();
        })
        done()
        console.log(TAG + "************* testRdbStoreDistributed002 end *************");
    })

    /**
     * @tc.name set distributed table using one table name
     * @tc.number SUB_DDM_AppDataFWK_JSRDB_Distributed_003
     * @tc.desc set distributed table using one table name
     */
    it('testRdbStoreDistributed0003', 0, async function (done) {
        console.log(TAG + "************* testRdbStoreDistributed003 start *************");
        let promise = rdbStore.setDistributedTables(['employee'])
        promise.then(() => {
            console.log(TAG + "set employee to be distributed table success");
            expect(rdbStore).assertEqual(rdbStore)
        }).catch(() => {
            console.log(TAG + "set employee to be distributed table failed");
            expect(null).assertFail();
        })
        done()
        console.log(TAG + "************* testRdbStoreDistributed003 end *************");
    })

    /**
     * @tc.name set distributed table using two table name
     * @tc.number SUB_DDM_AppDataFWK_JSRDB_Distributed_004
     * @tc.desc set distributed table using two table name
     */
    it('testRdbStoreDistributed0004', 0, async function (done) {
        console.log(TAG + "************* testRdbStoreDistributed004 start *************");
        let promise = rdbStore.setDistributedTables(['employee', 'product'])
        promise.then(() => {
            console.log(TAG + "set employee and product to be distributed table success");
            expect(rdbStore).assertEqual(rdbStore)
        }).catch(() => {
            console.log(TAG + "set employee and product to be distributed table failed");
            expect(null).assertFail();
        })
        done()
        console.log(TAG + "************* testRdbStoreDistributed004 end *************");
    })
    console.log(TAG + "*************Unit Test End*************");

    /**
     * @tc.name insert record after setting distributed table
     * @tc.number SUB_DDM_AppDataFWK_JSRDB_Distributed_005
     * @tc.desc insert record after setting distributed table
     */
    it('testRdbStoreDistributed0005', 0, async function (done) {
        console.log(TAG + "************* testRdbStoreDistributed005 start *************");
        const record = {
            "name": "Jim",
            "age": 20,
            "salary": 10000.0,
            "data": [1, 2, 3],
        }
        let promise = rdbStore.insert("employee", record)
        promise.then((rowId) => {
            console.log(TAG + "insert one record success " + rowId)
            expect(1).assertEqual(rowId)
        }).catch(() => {
            console.log(TAG + "insert one record failed");
            expect(null).assertFail();
        })
        done()
        console.log(TAG + "************* testRdbStoreDistributed005 end *************");
    })
    console.log(TAG + "*************Unit Test End*************");
})
