import aiosqlite
from typing import Dict, Any, Optional, Set, List, Tuple, Union
import datetime

DB_PATH = "LocalDB.db"
_schema_cache: Dict[str, Set[str]] = {}

def infer_sqlite_type(value: Any) -> str:
    if isinstance(value, bool):
        return "BOOLEAN"
    if isinstance(value, int):
        return "INTEGER"
    if isinstance(value, float):
        return "REAL"
    return "TEXT"

async def get_table_columns(table: str) -> Set[str]:
    if table in _schema_cache:
        return _schema_cache[table]
    async with aiosqlite.connect(DB_PATH) as db:
        rows = await db.execute_fetchall(f'PRAGMA table_info("{table}")')
        columns = {row[1] for row in rows}
        _schema_cache[table] = columns
        return columns

async def ensure_table(table: str, data: Dict[str, Any]) -> None:
    columns = await get_table_columns(table)
    missing = [k for k in data if k not in columns]
    if not columns:
        defs = [f'{k} {infer_sqlite_type(v)} DEFAULT NULL' for k, v in data.items()]
        sql = f'CREATE TABLE IF NOT EXISTS "{table}" (id INTEGER PRIMARY KEY AUTOINCREMENT, {", ".join(defs)})'
        await execute_db(sql, commit=True)
        _schema_cache[table] = set(data.keys()) | {"id"}
    else:
        for k in missing:
            sql = f'ALTER TABLE "{table}" ADD COLUMN {k} TEXT DEFAULT NULL'
            await execute_db(sql, commit=True)
            _schema_cache[table].add(k)

async def execute_db(
    sql: str, params: Tuple = (), fetch: Optional[str] = None, commit: bool = False
):
    async with aiosqlite.connect(DB_PATH) as db:
        cursor = await db.execute(sql, params)
        result = None
        if fetch == "one":
            row = await cursor.fetchone()
            if row:
                result = dict(zip([col[0] for col in cursor.description], row))
        elif fetch == "all":
            rows = await cursor.fetchall()
            result = [
                dict(zip([col[0] for col in cursor.description], row)) for row in rows
            ]
        if commit:
            await db.commit()
        if fetch:
            return result
        return cursor

async def create_db(table: str, data: Dict[str, Any]) -> int:
    data = {"created_at": datetime.datetime.now().isoformat(sep=' '), **data}
    await ensure_table(table, data)
    keys = ', '.join(data.keys())
    placeholders = ', '.join(['?'] * len(data))
    sql = f'INSERT INTO "{table}" ({keys}) VALUES ({placeholders})'
    cursor = await execute_db(sql, tuple(data.values()), commit=True)
    return cursor.lastrowid

async def read_db(
    table: str,
    row_id: Optional[int] = None,
    uuid: Optional[str] = None,  # <-- add this
    limit: int = 5,
    page: int = 1,
    start_date: Optional[str] = None,
    end_date: Optional[str] = None,
) -> Optional[Union[Dict[str, Any], List[Dict[str, Any]]]]:
    if row_id is not None:
        sql = f'SELECT * FROM "{table}" WHERE id=?'
        return await execute_db(sql, (row_id,), fetch="one")
    elif uuid is not None:
        sql = f'SELECT * FROM "{table}" WHERE uuid=?'
        return await execute_db(sql, (uuid,), fetch="one")
    else:
        offset = (page - 1) * limit
        sql = f'SELECT * FROM "{table}"'
        params = []
        where_clauses = []
        if start_date:
            where_clauses.append('created_at >= ?')
            params.append(start_date)
        if end_date:
            where_clauses.append('created_at <= ?')
            params.append(end_date)
        if where_clauses:
            sql += ' WHERE ' + ' AND '.join(where_clauses)
        sql += ' ORDER BY created_at DESC LIMIT ? OFFSET ?'
        params.extend([limit, offset])
        return await execute_db(sql, tuple(params), fetch="all")

async def update_db(table: str, row_id: int, data: Dict[str, Any]) -> bool:
    await ensure_table(table, data)
    set_clause = ', '.join([f"{key}=?" for key in data])
    sql = f'UPDATE "{table}" SET {set_clause} WHERE id=?'
    cursor = await execute_db(sql, tuple(data.values()) + (row_id,), commit=True)
    return cursor.rowcount > 0

async def delete_db(table: str, row_id: int) -> bool:
    sql = f'DELETE FROM "{table}" WHERE id=?'
    cursor = await execute_db(sql, (row_id,), commit=True)
    return cursor.rowcount > 0

async def main():
    user = {"username": "alice", "age": 25, "memory": "Hello, world!"}
    user_id = await create_db("users", user)
    print("Inserted user id:", user_id)
    print("User data:", await read_db("users", user_id))
    await update_db("users", user_id, {"age": 26})
    print("Updated data:", await read_db("users", user_id))
    
    # getting last 2 users
    users = await read_db("users", limit=2, page=1)
    print("Last 2 users:", users)
    
if __name__ == "__main__":
    import asyncio
    asyncio.run(main())