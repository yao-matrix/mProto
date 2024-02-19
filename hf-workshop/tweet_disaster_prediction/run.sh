
python training_script_cpu.py \
        --data_path=mehdiiraqui/twitter_disaster \
        --lora_alpha=16 \
        --lora_bias=none \
        --lora_dropout=0.1 \
        --lora_rank=2 \
        --lr=0.0001 \
        --max_length=512 \
        --model_name=mistralai/Mistral-7B-v0.1 \
        --num_epochs=10 \
        --output_path=mistral-lora-token-classification \
        --train_batch_size=8 \
        --eval_batch_size=8 \
        --weight_decay=0.001 \
        --set_pad_id
