import json
import os
from pathlib import Path
import struct

ROOT=Path(__file__).resolve().parents[2]
FROZEN=ROOT/'checkpoints/2026-10-28/packages/kitchen_recipes.krc'
FROZEN_META=ROOT/'model/tests/fixtures/kitchen_metadata_30.c'

def dictionary(data):
    offset,length=struct.unpack_from('<II',data,20)
    return json.loads(data[offset:offset+length])

def selected():
    recipe=os.environ.get('KITCHEN_RECIPES_KRC')
    meta=os.environ.get('KITCHEN_METADATA_C')
    if bool(recipe)!=bool(meta):
        raise RuntimeError('Set both KITCHEN_RECIPES_KRC and KITCHEN_METADATA_C')
    if recipe and len(dictionary(Path(recipe).read_bytes())['ids'])>64:
        raise RuntimeError('The design model supports at most 64 recipes; test larger packages in the expanded firmware model')
    return (Path(recipe),Path(meta)) if recipe else None

def recipe_ids(path, complete=False):
    ids=dictionary(path.read_bytes())['ids']
    if len(ids)!=len(set(ids)):
        raise AssertionError('duplicate recipe IDs')
    if complete:
        available={p.stem for p in (ROOT/'data/recipes/curated').glob('*.json')}
        if set(ids)!=available:
            raise AssertionError(f'KRC and curated recipe sets differ: missing={available-set(ids)}, extra={set(ids)-available}')
    return ids
