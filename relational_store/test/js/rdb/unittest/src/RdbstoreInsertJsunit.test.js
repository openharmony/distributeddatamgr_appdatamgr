/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
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

const TAG = "[RDB_JSKITS_TEST]"
const CREATE_TABLE_TEST = "CREATE TABLE IF NOT EXISTS test (" + "id INTEGER PRIMARY KEY AUTOINCREMENT, " + "name TEXT NOT NULL, " + "age INTEGER, " + "salary REAL, " + "blobType BLOB)";

const STORE_CONFIG = {
    name: "InsertTest.db",
}

var rdbStore = undefined;

describe('rdbStoreInsertTest', function () {
    beforeAll(async function () {
        console.info(TAG + 'beforeAll')
        rdbStore = await dataRdb.getRdbStore(STORE_CONFIG, 1);
        await rdbStore.executeSql(CREATE_TABLE_TEST, null);
    })

    beforeEach(async function () {
        console.info(TAG + 'beforeEach')
        await rdbStore.executeSql("DELETE FROM test");
    })

    afterEach(async function () {
        console.info(TAG + 'afterEach')
    })

    afterAll(async function () {
        console.info(TAG + 'afterAll')
        rdbStore = null
        await dataRdb.deleteRdbStore("InsertTest.db");
    })

    console.log(TAG + "*************Unit Test Begin*************");

    /**
     * @tc.name rdb insert test
     * @tc.number SUB_DDM_AppDataFWK_JSRDB_Insert_0001
     * @tc.desc rdb insert test
     */
    it('testRdbStoreInsert0001', 0, async function (done) {
        console.log(TAG + "************* testRdbStoreInsert0001 start *************");
        var u8 = new Uint8Array([1, 2, 3])
        {
            const valueBucket = {
                "name": "zhangsan",
                "age": 18,
                "salary": 100.5,
                "blobType": u8,
            }
            await rdbStore.insert("test", valueBucket)
        }
        {
            const valueBucket = {
                "name": "lisi",
                "age": 18,
                "salary": 100.5,
                "blobType": u8,
            }
            await rdbStore.insert("test", valueBucket)
        }
        {
            const valueBucket = {
                "name": "lisi",
                "age": 20,
                "salary": 100.5,
                "blobType": u8,
            }
            await rdbStore.insert("test", valueBucket)
        }

        let predicates = new dataRdb.RdbPredicates("test");
        predicates.equalTo("name", "zhangsan")
        let resultSet = await rdbStore.query(predicates)
        try {
            console.log(TAG + "resultSet query done");
            expect(true).assertEqual(resultSet.goToFirstRow())
            const id = resultSet.getLong(resultSet.getColumnIndex("id"))
            const name = resultSet.getString(resultSet.getColumnIndex("name"))
            const age = resultSet.getLong(resultSet.getColumnIndex("age"))
            const salary = resultSet.getDouble(resultSet.getColumnIndex("salary"))
            const blobType = resultSet.getBlob(resultSet.getColumnIndex("blobType"))
            console.log(TAG + "id=" + id + ", name=" + name + ", age=" + age + ", salary=" + salary + ", blobType=" + blobType);
            expect(1).assertEqual(id);
            expect("zhangsan").assertEqual(name)
            expect(18).assertEqual(age)
            expect(100.5).assertEqual(salary)
            expect(1).assertEqual(blobType[0])
            expect(2).assertEqual(blobType[1])
            expect(3).assertEqual(blobType[2])
            expect(false).assertEqual(resultSet.goToNextRow())
        } catch (e) {
            console.log("insert1 error " + e);
        }
        resultSet = null
        done()
        console.log(TAG + "************* testRdbStoreInsert0001 end *************");
    })

    /**
     * @tc.name rdb insert test
     * @tc.number SUB_DDM_AppDataFWK_JSRDB_Insert_0002
     * @tc.desc rdb insert test
     */
    it('testRdbStoreInsert0002', 0, async function (done) {
        console.log(TAG + "************* testRdbStoreInsert0002 start *************");
        var u8 = new Uint8Array([1, 2, 3])
        {
            const valueBucket = {
                "name": "zhangsan",
                "age": 18,
                "salary": 100.5,
                "blobType": u8,
            }
            let insertPromise = rdbStore.insert("wrong", valueBucket)
            insertPromise.then(async (ret) => {
                expect(1).assertEqual(ret)
                console.log(TAG + "insert first done: " + ret)
                expect(null).assertFail()
            }).catch((err) => {
                console.log(TAG + "insert with wrong table")
            })
        }
        done()
        console.log(TAG + "************* testRdbStoreInsert0002 end   *************");
    })

    /**
     * @tc.name rdb insert test
     * @tc.number SUB_DDM_AppDataFWK_JSRDB_Insert_0003
     * @tc.desc rdb insert test
     */
    it('testRdbStoreInsert0003', 0, async function (done) {
        console.log(TAG + "************* testRdbStoreInsert0003 start *************");
        var u8 = new Uint8Array([1, 2, 3])
        {
            const valueBucket = {
                "name": "zhangsan",
                "age": 18,
                "salary": 100.5,
                "blobType": u8,
            }
            let insertPromise = rdbStore.insert(null, valueBucket)
            insertPromise.then(async (ret) => {
                expect(1).assertEqual(ret)
                console.log(TAG + "insert first done: " + ret)
                expect(null).assertFail()
            }).catch((err) => {
                console.log(TAG + "insert with null table")
            })
        }
        done()
        console.log(TAG + "************* testRdbStoreInsert0003 end   *************");
    })

    /**
     * @tc.name rdb insert Extra long character test
     * @tc.number SUB_DDM_AppDataFWK_JSRDB_Insert_0004
     * @tc.desc rdb insert Extra long character test
     */
    it('testRdbStoreInsert0004', 0, async function (done) {
        console.log(TAG + "************* testRdbStoreInsert0004 start *************");
        var u8 = new Uint8Array([1, 2, 3])
        var nameStr = "abcd" + "e".repeat(2000) + "./&*$!@()"
        const valueBucket = {
            "name": nameStr,
            "age": 19,
            "salary": 100.5,
            "blobType": u8,
        }
        await rdbStore.insert("test", valueBucket)
        let predicates = new dataRdb.RdbPredicates("test");
        predicates.equalTo("age", 19)
        let resultSet = await rdbStore.query(predicates)
        try {
            console.log(TAG + "resultSet query done");
            expect(true).assertEqual(resultSet.goToFirstRow())
            const name = resultSet.getString(resultSet.getColumnIndex("name"))
            console.log(TAG + "id=" + id + ", name=" + name + ", age=" + age + ", salary=" + salary + ", blobType=" + blobType);
            expect(nameStr).assertEqual(name)
        } catch (e) {
            console.log("insert error " + e);
        }
        resultSet = null
        done()
        console.log(TAG + "************* testRdbStoreInsert0004 end   *************");
    })

    /**
     * @tc.name rdb insert Extra long character test
     * @tc.number SUB_DDM_AppDataFWK_JSRDB_Insert_0005
     * @tc.desc rdb insert Extra long character test
     */
    it('testRdbStoreInsert0005', 0, async function (done) {
        console.log(TAG + "************* testRdbStoreInsert0005 start *************");
        var u8 = new Uint8Array([1, 2, 3])
        var nameStr = "苹果是水果" + "e".repeat(2000)
        const valueBucket = {
            "name": nameStr,
            "age": 20,
            "salary": 100.5,
            "blobType": u8,
        }
        await rdbStore.insert("test", valueBucket)
        let predicates = new dataRdb.RdbPredicates("test");
        predicates.equalTo("age", 20)
        let resultSet = await rdbStore.query(predicates)
        try {
            console.log(TAG + "resultSet query done");
            expect(true).assertEqual(resultSet.goToFirstRow())
            const name = resultSet.getString(resultSet.getColumnIndex("name"))
            console.log(TAG + "id=" + id + ", name=" + name + ", age=" + age + ", salary=" + salary + ", blobType=" + blobType);
            expect(nameStr).assertEqual(name)
        } catch (e) {
            console.log("insert error " + e);
        }
        resultSet = null
        done()
        console.log(TAG + "************* testRdbStoreInsert0005 end   *************");
    })

    /**
     * @tc.name rdb insert Extra long character test
     * @tc.number SUB_DDM_AppDataFWK_JSRDB_Insert_0006
     * @tc.desc rdb insert Extra long character test
     */
    it('testRdbStoreInsert0006', 0, async function (done) {
        console.log(TAG + "************* testRdbStoreInsert0006 start *************");
        var u8 = new Uint8Array([1, 2, 3])
        var nameStr = "西瓜是水果" + "e".repeat(2000) + "好吃又好看"
        const valueBucket = {
            "name": nameStr,
            "age": 21,
            "salary": 100.5,
            "blobType": u8,
        }
        await rdbStore.insert("test", valueBucket)
        let predicates = new dataRdb.RdbPredicates("test");
        predicates.equalTo("age", 21)
        let resultSet = await rdbStore.query(predicates)
        try {
            console.log(TAG + "resultSet query done");
            expect(true).assertEqual(resultSet.goToFirstRow())
            const name = resultSet.getString(resultSet.getColumnIndex("name"))
            console.log(TAG + "id=" + id + ", name=" + name + ", age=" + age + ", salary=" + salary + ", blobType=" + blobType);
            expect(nameStr).assertEqual(name)
        } catch (e) {
            console.log("insert error " + e);
        }
        resultSet = null
        done()
        console.log(TAG + "************* testRdbStoreInsert0006 end   *************");
    })

    /**
     * @tc.name rdb batchInsert test
     * @tc.number SUB_DDM_AppDataFWK_JSRDB_batchInsert_0001
     * @tc.desc rdb batchInsert test
     */
    it('testRdbStorebatchInsert001', 0, async function () {
        console.log(TAG + "************* testRdbStorebatchInsert001 start *************");

        var u8 = new Uint8Array([1, 2, 3])
        const valueBucket = {
            "name": "zhangsan",
            "age": 18,
            "salary": 100.5,
            "blobType": u8,
        }
        let valueBucketArray = new Array();
        for (let i = 0; i < 100; i++) {
            valueBucketArray.push(valueBucket);
        }
        await rdbStore.batchInsert("test", valueBucketArray);
        let resultSet = await rdbStore.querySql("SELECT * FROM test");
        let count = resultSet.rowCount;
        expect(100).assertEqual(count);
        console.log(TAG + "************* testRdbStorebatchInsert001 end *************");
    })

    console.log(TAG + "*************Unit Test End*************");
})