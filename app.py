import streamlit as st

st.set_page_config(page_title="OmniBharat Core Research Assistant", layout="wide")

st.title("🇮🇳 OmniBharat Core Research Assistant")
st.markdown("### Powered by Team Lead Intelligence | Hackathon Research Dashboard")

# Sidebar for categories
category = st.sidebar.selectbox(
    "Choose Research Category",
    ["Select Category", "Farmers (Krishi)", "Women Empowerment", "Youth Skills", "Student Education"]
)

if category == "Farmers (Krishi)":
    st.header("🌾 Farmers Research & Schemes")
    st.write("Real-world ground data and government schemes for agricultural support.")
    
    st.table({
        "Scheme Name": ["PM-KISAN", "PM Fasal Bima Yojana", "Kisan Credit Card (KCC)"],
        "Target": ["All Landholding Farmers", "Farmers facing crop loss", "Farmers needing cheap credit"],
        "Core Benefit": ["₹6,000/year direct transfer", "Crop insurance coverage", "Low-interest institutional loans"],
        "Ground Problem": ["Delayed payments & awareness gap", "Claim settlement transparency", "High middleman interference"]
    })

elif category == "Women Empowerment":
    st.header("👩‍🦰 Women Empowerment & Welfare")
    st.write("Financial security, safety, and entrepreneurial support schemes.")
    
    st.table({
        "Scheme Name": ["Mukhya Mantri Ladli Behna", "Stand-Up India", "PM Matru Vandana Yojana"],
        "Target": ["Married Women (21-60 yrs)", "Women Entrepreneurs", "First-time Pregnant Mothers"],
        "Core Benefit": ["Monthly financial assistance", "Bank loans ₹10L to ₹1Cr", "Nutrition & cash support"],
        "Ground Problem": ["Complex paperwork & documentation", "Lack of collateral/guarantee knowledge", "Portal technical glitches"]
    })

elif category == "Youth Skills":
    st.header("⚡ Youth Skill Development")
    st.write("Employment, vocational training, and startup ecosystem insights.")
    
    st.table({
        "Scheme Name": ["PM Kaushal Vikas Yojana (PMKVY)", "Startup India Initiative", "National Apprenticeship Training"],
        "Target": ["Unemployed Youth / School dropouts", "Early-stage founders", "Graduates & Diploma holders"],
        "Core Benefit": ["Industry-standard skill certification", "Tax exemptions & funding support", "Stipend-based on-job training"],
        "Ground Problem": ["Job placement conversion rates", "Lack of mentoring in tier-2/3 cities", "Awareness about funding windows"]
    })

elif category == "Student Education":
    st.header("🎓 Student Education & Guidance")
    st.write("Scholarships, career paths, and technical learning resources.")
    
    st.table({
        "Scheme Name": ["National Scholarship Portal (NSP)", "PM Flagship Higher Education Loans", "AICTE Internships"],
        "Target": ["Meritorious & reserved category students", "Students pursuing higher education", "Engineering & technical students"],
        "Core Benefit": ["Direct scholarship disbursement", "Collateral-free education loans", "Verified industry internships"],
        "Ground Problem": ["Verification delays", "Strict eligibility barriers", "Mismatch with rural student access"]
    })

else:
    st.info("👈 Left sidebar se koi bhi category select karo taaki research data aur structured tables saamne aa sakein!")
