import streamlit as st

st.set_page_config(page_title="OmniBharat AI Research Assistant", layout="centered")

st.title("🤖 OmniBharat AI Research Assistant")
st.markdown("### Ask anything about Farmers, Women, Youth & Student Schemes")

# Initialize chat history
if "messages" not in st.session_state:
    st.session_state.messages = []

# Display chat messages from history on app rerun
for message in st.session_state.messages:
    with st.chat_message(message["role"]):
        st.markdown(message["content"])

# React to user input
if prompt := st.chat_input("Aapko kis category ya scheme ke baare mein janna hai?"):
    # Display user message in chat message container
    st.session_state.messages.append({"role": "user", "content": prompt})
    with st.chat_message("user"):
        st.markdown(prompt)

    # Generate AI response based on query
    response = ""
    prompt_lower = prompt.lower()
    
    if "farmer" in prompt_lower or "krishi" in prompt_lower or "kisan" in prompt_lower:
    axs    response = "🌾 **Farmers Schemes & Insights:**\n- **PM-KISAN:** ₹6,000/year direct transfer.\n- **PM Fasal Bima Yojana:** Crop insurance coverage.\n- **Ground Problem:** Delayed payments and awareness gap in rural areas."
    elif "women" in prompt_lower or "mahila" in prompt_lower or "ladli" in prompt_lower:
        response = "👩‍🦰 **Women Empowerment Schemes:**\n- **Ladli Behna Yojana:** Monthly financial assistance.\n- **Stand-Up India:** Business loans from ₹10L to ₹1Cr.\n- **Ground Problem:** Complex paperwork and digital illiteracy."
    elif "youth" in prompt_lower or "startup" in prompt_lower or "skill" in prompt_lower:
        response = "⚡ **Youth Skill & Startup Schemes:**\n- **PMKVY:** Industry skill certification.\n- **Startup India:** Tax exemptions and funding.\n- **Ground Problem:** Lack of local mentorship in Tier-3 cities."
    elif "student" in prompt_lower or "education" in prompt_lower or "scholarship" in prompt_lower:
        response = "🎓 **Student Education Schemes:**\n- **NSP (National Scholarship Portal):** Direct scholarship disbursement.\n- **Education Loans:** Collateral-free higher education support.\n- **Ground Problem:** Portal verification delays."
    else:
        response = f"Bhai, maine aapka sawaal ('{prompt}') sun liya hai. OmniBharat project ke liye aap Farmers, Women, Youth ya Students ki categories ke baare mein pooch sakte hain. Inmein se kis par deep research chahiye?"

    # Display assistant response in chat message container
    with st.chat_message("assistant"):
        st.markdown(response)
    st.session_state.messages.append({"role": "assistant", "content": response})
